#include "exec/Executor.hpp"

#include "arithmetic/ArithmeticEngine.hpp"
#include "arithmetic/ArithmeticException.hpp"
#include "builtins/BuiltInFunction.hpp"
#include "builtins/BuiltinDispatcher.hpp"
#include "exec/ExecException.hpp"
#include "exec/ExecHelpers.hpp"
#include "exec/ProcessExecutor.hpp"
#include "exec/Redirection.hpp"
#include "exec/SuspendedCoro.hpp"
#include "exec/WaitStatus.hpp"
#include "io/FdOps.hpp"
#include "io/IOException.hpp"
#include "io/Pipe.hpp"
#include "parser/ast/Redirect.hpp"
#include "parser/ast/commands/AndOr.hpp"
#include "parser/ast/commands/ArithmeticCommand.hpp"
#include "parser/ast/commands/CStyleFor.hpp"
#include "parser/ast/commands/Case.hpp"
#include "parser/ast/commands/For.hpp"
#include "parser/ast/commands/Function.hpp"
#include "parser/ast/commands/Group.hpp"
#include "parser/ast/commands/If.hpp"
#include "parser/ast/commands/List.hpp"
#include "parser/ast/commands/Pipeline.hpp"
#include "parser/ast/commands/SimpleCommand.hpp"
#include "parser/ast/commands/Subshell.hpp"
#include "parser/ast/commands/While.hpp"
#include "shell/JobTable.hpp"
#include "shell/ShellArithmeticVars.hpp"
#include "shell/ShellState.hpp"
#include "shell/expander/Expander.hpp"
#include "signals/SignalManager.hpp"
#include "utils/ErrorCodes.hpp"

#include <csignal>
#include <fnmatch.h>
#include <memory>
#include <syslog.h>
#include <unistd.h>

namespace
{

struct CompoundScope
{
    bool& flag;
    bool prev;

    explicit CompoundScope(bool& target) : flag(target), prev(target)
    {
        flag = true;
    }

    ~CompoundScope()
    {
        flag = prev;
    }

    CompoundScope(const CompoundScope&) = delete;
    CompoundScope& operator=(const CompoundScope&) = delete;
};

struct SuspendAwaitable
{
    std::coroutine_handle<>& slot;
    int& statusSlot;

    bool await_ready() noexcept
    {
        return false;
    }

    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<> self) noexcept
    {
        slot = self;
        return std::noop_coroutine();
    }

    int await_resume() noexcept
    {
        return statusSlot;
    }
};

void SetupChildFds(
    const std::vector<std::unique_ptr<io::Pipe>>& pipes,
    size_t idx,
    size_t nStages,
    const std::vector<parser::ast::Redirect>& redirects)
{
    if (idx > 0)
        io::fdops::Dup2(*pipes[idx - 1]->GetReadPipeFD(),
                        STDIN_FILENO);
    if (idx < nStages - 1)
        io::fdops::Dup2(*pipes[idx]->GetWritePipeFD(), STDOUT_FILENO);
    for (const auto& redir : redirects)
    {
        exec::ApplyRedirect(redir);
    }
}

std::unique_ptr<io::Pipe> CreatePipe()
{
    try
    {
        return std::make_unique<io::Pipe>();
    }
    catch (const io::IOException& ex)
    {
        syslog(LOG_ERR,
               "SHELL [exec] [Pipeline]: Failed to create pipe: %s "
               "(errno=%d)",
               ex.what(),
               ex.GetErrorCode());
        throw exec::ExecException("failed To create pipe",
                                  FAILED_TO_CREATE);
    }
}
} // namespace

namespace exec
{
Executor::Executor(
    std::unique_ptr<shell::ShellState>& state,
    std::unique_ptr<builtins ::BuiltinDispatcher>& builtins,
    const shell::expander::CommandRunner& cmdRunner,
    int outFd)
    : m_state_(state)
    , m_builtins_(builtins)
    , m_cmdRunner_(cmdRunner)
    , m_outFd_(outFd)
    , m_suspendedHandle_({})
    , m_suspendedPgid_(-1)
    , m_resumedStatus_(-1)
{
}

int Executor::Run(const std::shared_ptr<parser::ast::AstNode>& root)
{
    m_suspendedHandle_ = {};
    coro::Task task = root->Accept(*this);
    task.GetHandle().resume();
    if (m_suspendedHandle_)
    {
        std::shared_ptr<SuspendedCoro> suspended =
            std::make_shared<SuspendedCoro>(
                SuspendedCoro{.rootTask = std::move(task),
                              .leafHandle = m_suspendedHandle_,
                              .ast = root,
                              .resume = nullptr});

        // Park the Executor's resume logic on the job so fg can thaw
        // it without ever holding an Executor handle. rec stays valid
        // after the move below the job owns the record on the heap.
        SuspendedCoro* rec = suspended.get();
        suspended->resume =
            [this, rec](int leafStatus) -> SuspendedCoro::ResumeResult
        { return ResumeSuspended(*rec, leafStatus); };

        auto jobId = m_state_->GetJobs()->AddSuspended(
            m_suspendedPgid_,
            root->SourceText(),
            std::move(suspended),
            shell::JobTable::State::Stopped);
        std::string out = "[" + std::to_string(jobId) +
                          "]+ Stopped " + root->SourceText() + "\n";
        Announce(out);
        return SIGNAL_EXIT_BASE + SIGTSTP;
    }
    auto& promise = task.GetHandle().promise();
    if (auto excp = promise.Exception())
        std::rethrow_exception(excp);
    return promise.Result();
}

int Executor::RunToCompletion(
    const std::unique_ptr<parser::ast::AstNode>& node)
{
    coro::Task task = node->Accept(*this);
    task.GetHandle().resume();
    auto& promise = task.GetHandle().promise();
    if (auto excp = promise.Exception())
        std::rethrow_exception(excp);
    return promise.Result();
}

SuspendedCoro::ResumeResult
Executor::ResumeSuspended(SuspendedCoro& rec, int leafStatus)
{
    // Cleared first so a fresh freeze deeper in the compound is
    // detectable after the leaf resumes.
    m_suspendedHandle_ = {};
    // Drop the just-finished leaf's real status in the mailbox; the
    // thawed SuspendAwaitable::await_resume reads it as the value of
    // co_await.
    m_resumedStatus_ = leafStatus;
    rec.leafHandle.resume();

    if (m_suspendedHandle_)
    {
        // Ctrl-Z again: the compound re-froze on a new leaf. Re-point
        // the record at it and report the new process group up to fg.
        rec.leafHandle = m_suspendedHandle_;
        return {.completed = false,
                .status = SIGNAL_EXIT_BASE + SIGTSTP,
                .newPgid = m_suspendedPgid_};
    }

    // The whole compound ran out; surface its final status (and any
    // throw).
    auto& promise = rec.rootTask.GetHandle().promise();
    if (auto excp = promise.Exception())
        std::rethrow_exception(excp);
    return {.completed = true,
            .status = promise.Result(),
            .newPgid = -1};
}

CommandSpec
Executor::BuildSpec(const std::vector<std::string>& argv,
                    const parser::ast::SimpleCommand& command) const
{
    return CommandSpec(argv,
                       command.Redirects(),
                       command.Assignments());
}

void Executor::RecordStoppedJob(exec::State state,
                                pid_t pid,
                                const std::string& commandText)
{
    if (state != exec::State::Stopped)
        return;

    auto jobID =
        m_state_->GetJobs()->Add(pid,
                                 commandText,
                                 shell::JobTable::State::Stopped);
    std::string out = "[" + std::to_string(jobID) + "]+ Stopped " +
                      commandText + "\n";
    Announce(out);
}

void Executor::Announce(const std::string& line) const
{
    io::fdops::WriteAll(m_outFd_, line);
}

std::vector<std::string>
Executor::ExpandArgv(const std::vector<std::string>& argv)
{
    std::vector<std::string> out;
    for (const auto& word : argv)
    {
        auto pieces =
            shell::expander::Expand(word, m_state_, m_cmdRunner_);
        for (auto& piece : pieces)
            out.push_back(std::move(piece));
    }
    return out;
}

coro::Task Executor::Visit(parser::ast::SimpleCommand& command)
{
    auto argv = ExpandArgv(command.Argv());

    if (argv.empty())
    {
        for (const auto& assignment : command.Assignments())
            m_state_->SetVar(
                assignment.first,
                shell::expander::Expand(assignment.second,
                                        m_state_,
                                        m_cmdRunner_)
                    .front());
        m_status_ = 0;
        co_return m_status_;
    }
    if (auto* body = m_state_->GetFunctionBody(argv[0]))
    {
        co_await body->Accept(*this);
        co_return m_status_;
    }
    else if (m_builtins_->IsBuiltin(argv[0]))
    {
        auto ctx =
            std::make_unique<builtins::BuiltinContext>(m_state_);
        m_status_ = m_builtins_->Run(argv, ctx);
    }
    else
    {
        auto spec = BuildSpec(argv, command);
        if (m_inForkedChild_)
        {
            ProcessExecutor proc;
            proc.Exec(spec);
        }

        ProcessExecutor runner;
        auto& sigMgr = m_state_->GetSignalMgr();
        runner.Fork(
            [&]() -> int
            {
                sigMgr->ResetForChild();
                ProcessExecutor proc;
                proc.Exec(spec);
                return EXIT_FAILURE;
            },
            0);

        auto waitMode = m_state_->IsJobControlEnabled()
                            ? WaitMode::Foreground
                            : WaitMode::UntilExit;
        if (waitMode == WaitMode::Foreground)
            tcsetpgrp(STDIN_FILENO, runner.Pid());

        WaitStatus status(runner.Pid(), waitMode);

        if (waitMode == WaitMode::Foreground)
        {
            tcsetpgrp(STDIN_FILENO, getpgrp());
            m_state_->RestoreTerminalModes();
        }
        auto state =
            status.IsStopped() ? State::Stopped : State::Done;
        if (state == State::Stopped && m_inCompound_)
        {
            m_suspendedPgid_ = runner.Pid();
            m_status_ = co_await SuspendAwaitable{
                .slot = m_suspendedHandle_,
                .statusSlot = m_resumedStatus_};
        }
        else
        {
            RecordStoppedJob(state,
                             runner.Pid(),
                             command.SourceText());
        }
        if (state == State::Done)
        {
            if (status.Signaled())
                m_status_ = SIGNAL_EXIT_BASE + status.GetSignal();
            else if (status.Exited())
                m_status_ = status.ExitCode();
            else
                m_status_ = 0;
        }
    }
    co_return m_status_;
}

std::vector<ProcessExecutor> Executor::LaunchStages(
    const std::vector<std::unique_ptr<parser::ast::AstNode>>& stages,
    const std::vector<std::unique_ptr<io::Pipe>>& pipes)
{
    std::vector<ProcessExecutor> runners(stages.size());
    auto& sigMgr = m_state_->GetSignalMgr();
    pid_t pgid = 0;
    for (size_t idx = 0; idx < stages.size(); idx++)
    {
        runners[idx].Fork(
            [&, idx]()
            {
                sigMgr->ResetForChild();
                SetupChildFds(pipes,
                              idx,
                              stages.size(),
                              stages[idx]->Redirects());
                m_inForkedChild_ = true;
                RunToCompletion(stages[idx]);
                return m_status_;
            },
            pgid);
        pgid = runners[0].Pid();
    }
    return runners;
}

std::vector<std::unique_ptr<WaitStatus>>
Executor::WaitStages(const std::vector<ProcessExecutor>& runners,
                     WaitMode mode)
{
    if (mode == WaitMode::Foreground)
        tcsetpgrp(STDIN_FILENO, runners[0].Pid());

    std::vector<std::unique_ptr<WaitStatus>> statuses(runners.size());
    for (size_t i = 0; i < runners.size(); i++)
        statuses[i] =
            std::make_unique<WaitStatus>(runners[i].Pid(), mode);

    if (mode == WaitMode::Foreground)
    {
        tcsetpgrp(STDIN_FILENO, getpgrp());
        m_state_->RestoreTerminalModes();
    }

    return statuses;
}

int Executor::CollectStatus(
    const std::vector<std::unique_ptr<WaitStatus>>& statuses,
    bool pipefail)
{
    if (!pipefail)
    {
        const auto& status = statuses.back();
        if (status->IsValid())
        {
            if (status->Signaled())
                return SIGNAL_EXIT_BASE + status->GetSignal();
            else if (status->Exited())
                return status->ExitCode();
        }
        return 0;
    }
    for (const auto& status : statuses)
    {
        if (status->IsValid())
        {
            if (status->Signaled())
                return SIGNAL_EXIT_BASE + status->GetSignal();
            else if (status->Exited() && status->ExitCode() != 0)
                return status->ExitCode();
        }
    }
    return 0;
}

coro::Task Executor::Visit(parser::ast::Pipeline& pipeline)
{
    const auto& stages = pipeline.Stages();
    std::vector<std::unique_ptr<io::Pipe>> pipes;
    try
    {
        for (size_t i = 0; i < stages.size() - 1; i++)
            pipes.push_back(CreatePipe());
    }
    catch (const ExecException& ex)
    {
        throw ExecException(
            std::string("SHELL [exec] [Pipeline] : Failed to run "
                        "command: ") +
                ex.what() + " with errno: " + std::to_string(errno),
            ex.GetErrorCode());
    }

    auto runners = LaunchStages(stages, pipes);

    for (auto& pipe : pipes)
    {
        pipe->GetReadPipeFD()->Close();
        pipe->GetWritePipeFD()->Close();
    }

    auto waitMode = m_state_->IsJobControlEnabled()
                        ? WaitMode::Foreground
                        : WaitMode::UntilExit;
    auto statuses = WaitStages(runners, waitMode);

    auto state =
        statuses.back()->IsStopped() ? State::Stopped : State::Done;
    if (state == State::Stopped && m_inCompound_)
    {
        m_suspendedPgid_ = runners[0].Pid();
        m_status_ =
            co_await SuspendAwaitable{.slot = m_suspendedHandle_,
                                      .statusSlot = m_resumedStatus_};
    }
    else
    {
        RecordStoppedJob(state,
                         runners[0].Pid(),
                         pipeline.SourceText());
    }

    if (state == State::Done)
        m_status_ =
            CollectStatus(statuses,
                          m_state_->IsOptionEnabled("pipefail"));

    if (pipeline.Bang())
        m_status_ = (m_status_ == 0) ? 1 : 0;

    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::Subshell& node)
{
    ProcessExecutor runner;
    m_status_ = runner.Run(
        [&]
        {
            RunToCompletion(node.GetBody());
            return m_status_;
        },
        0,
        m_state_->IsJobControlEnabled() ? WaitMode::Foreground
                                        : WaitMode::Poll);
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::List& list)
{
    CompoundScope scope(m_inCompound_);
    const auto& items = list.GetItems();
    for (const auto& item : items)
    {
        if (item.background)
        {
            auto runner = ProcessExecutor();
            int errCode = runner.Run(
                [&]
                {
                    setpgid(0,
                            0); // own group -> kill(-pgid) reaches it
                    m_state_->EnableJobControl(false);
                    RunToCompletion(item.node);
                    return m_status_;
                },
                0,
                WaitMode::Poll);
            if (errCode == PROCESS_RUNNING)
            {
                pid_t pid = runner.Pid();
                if (pid > 0)
                {
                    setpgid(pid, pid); // parent half of the
                                       // race-guarded setpgid
                    auto& jobTable = m_state_->GetJobs();
                    auto id =
                        jobTable->Add(pid, item.node->SourceText());
                    m_status_ = 0;
                    std::string err = "[" + std::to_string(id) +
                                      "] " + std::to_string(pid) +
                                      "\n";
                    write(STDOUT_FILENO, err.c_str(), err.size());
                }
            }
        }
        else
        {
            co_await item.node->Accept(*this);
        }
    }
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::AndOr& command)
{
    CompoundScope scope(m_inCompound_);
    co_await command.Lhs()->Accept(*this);
    if ((m_status_ == 0 &&
         (command.Operator() == parser::ast::AndOr::Op::And)))
    {
        co_await command.Rhs()->Accept(*this);
    }
    else if (m_status_ != 0 &&
             (command.Operator() == parser::ast::AndOr::Op::Or))
    {
        co_await command.Rhs()->Accept(*this);
    }
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::Group& group)
{
    CompoundScope scope(m_inCompound_);
    co_await group.GetBody()->Accept(*this);
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::While& condi)
{
    CompoundScope scope(m_inCompound_);
    while (true)
    {
        co_await condi.GetCondition()->Accept(*this);
        bool keepGoing = (m_status_ == 0);
        if (condi.IsUntil())
        {
            keepGoing = !keepGoing;
        }
        if (!keepGoing)
        {
            break;
        }
        co_await condi.GetBody()->Accept(*this);
    }
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::For& loop)
{
    CompoundScope scope(m_inCompound_);
    const auto& words = loop.GetWords();
    for (const auto& word : words)
    {
        m_state_->SetVar(loop.GetVar(), word);
        co_await loop.GetBody()->Accept(*this);
    }
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::CStyleFor& loop)
{
    CompoundScope scope(m_inCompound_);

    if (loop.GetInit())
        co_await loop.GetInit()->Accept(*this);

    while (true)
    {
        if (loop.GetCond())
        {
            co_await loop.GetCond()->Accept(*this);
            if (m_status_ != 0)
                break;
        }

        if (loop.GetBody())
            co_await loop.GetBody()->Accept(*this);

        if (loop.GetUpdate())
            co_await loop.GetUpdate()->Accept(*this);
    }
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::If& condi)
{
    CompoundScope scope(m_inCompound_);
    const auto& branches = condi.GetBranches();
    for (const auto& branch : branches)
    {
        co_await branch.condition->Accept(*this);
        if (m_status_ == 0)
        {
            co_await branch.body->Accept(*this);
            co_return m_status_;
        }
    }
    if (condi.GetElseBody())
    {
        co_await condi.GetElseBody()->Accept(*this);
    }
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::Case& case_)
{
    CompoundScope scope(m_inCompound_);
    const auto& word = case_.GetWord();
    const auto& arms = case_.GetArms();
    for (const auto& arm : arms)
    {
        for (const auto& pattern : arm.patterns)
        {
            if (fnmatch(pattern.c_str(), word.c_str(), 0) == 0)
            {
                if (arm.body != nullptr)
                {
                    co_await arm.body->Accept(*this);
                }
                co_return m_status_;
                ;
            }
        }
    }
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::Function& node)
{
    m_state_->AddFunction(node.GetName(), node.ReleaseBody());
    m_status_ = 0;
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::ArithmeticCommand& node)
{
    shell::ShellArithmeticVars adapter(m_state_);
    try
    {
        auto result =
            arithmetic::engine::Evaluate(node.GetExpr(), adapter);
        m_status_ = (result != 0) ? 0 : 1;
        co_return m_status_;
    }
    catch (const arithmetic::ArithmeticException& ex)
    {
        m_status_ = 1;
        // to do log the error when logger is intergrated
        co_return m_status_;
    }
}
} // namespace exec