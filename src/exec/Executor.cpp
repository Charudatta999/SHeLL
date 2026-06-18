#include "exec/Executor.hpp"

#include "arithmetic/ArithmeticEngine.hpp"
#include "arithmetic/ArithmeticException.hpp"
#include "builtins/BuiltInFunction.hpp"
#include "builtins/BuiltinDispatcher.hpp"
#include "exec/ExecHelpers.hpp"
#include "exec/ForkRunner.hpp"
#include "exec/Pipeline.hpp"
#include "exec/SuspendedCoro.hpp"
#include "exec/WaitStatus.hpp"
#include "io/FdOps.hpp"
#include "parser/ast/commands/AndOr.hpp"
#include "parser/ast/commands/ArithmeticCommand.hpp"
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
#include "utils/ErrorCodes.hpp"

#include <csignal>
#include <fnmatch.h>
#include <memory>
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

    int await_resume() noexcept {return statusSlot;}
};

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

        // Park the Executor's resume logic on the job so fg can thaw it
        // without ever holding an Executor handle. rec stays valid after
        // the move below the job owns the record on the heap.
        SuspendedCoro* rec = suspended.get();
        suspended->resume =
            [this, rec](int leafStatus) -> SuspendedCoro::ResumeResult
        { return ResumeSuspended(*rec, leafStatus); };

        auto jobId = m_state_->GetJobs()->AddSuspended(
            m_suspendedPgid_,
            root->SourceText(),
            std::move(suspended),
            shell::JobTable::State::Stopped);
        std::string out = "[" + std::to_string(jobId) + "]+ Stopped " + root->SourceText() + "\n";
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
    // Cleared first so a fresh freeze deeper in the compound is detectable
    // after the leaf resumes.
    m_suspendedHandle_ = {};
    // Drop the just-finished leaf's real status in the mailbox; the thawed
    // SuspendAwaitable::await_resume reads it as the value of co_await.
    m_resumedStatus_ = leafStatus;
    rec.leafHandle.resume();

    if (m_suspendedHandle_)
    {
        // Ctrl-Z again: the compound re-froze on a new leaf. Re-point the
        // record at it and report the new process group up to fg.
        rec.leafHandle = m_suspendedHandle_;
        return {.completed = false,
                .status = SIGNAL_EXIT_BASE + SIGTSTP,
                .newPgid = m_suspendedPgid_};
    }

    // The whole compound ran out; surface its final status (and any throw).
    auto& promise = rec.rootTask.GetHandle().promise();
    if (auto excp = promise.Exception())
        std::rethrow_exception(excp);
    return {.completed = true, .status = promise.Result(), .newPgid = -1};
}

CommandSpec
Executor::BuildSpec(const std::vector<std::string>& argv,
                    const parser::ast::SimpleCommand& command) const
{
    return CommandSpec(argv,
                       command.Redirects(),
                       command.Assignments());
}

void Executor::RecordStoppedJob(PipelineResult result,
                                const std::string& commandText)
{
    if (result.state != exec::State::Stopped)
        return;

    auto jobID =
        m_state_->GetJobs()->Add(result.pgid,
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
        auto pipeline = exec::Pipeline();
        auto result = pipeline.Run(
            {spec},
            m_state_->IsOptionEnabled("pipefail"),
            m_state_->IsJobControlEnabled() ? WaitMode::Foreground
                                            : WaitMode::UntilExit);
        m_status_ = result.status;
        if (result.state == exec::State::Stopped && m_inCompound_)
        {
            m_suspendedPgid_ = result.pgid;
            m_status_ = co_await SuspendAwaitable{.slot = m_suspendedHandle_, .statusSlot = m_resumedStatus_};
        }
        else
        {
            RecordStoppedJob(result, command.SourceText());
        }
    }
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::Pipeline& pipeline)
{
    const auto& stages = pipeline.Stages();
    std::vector<CommandSpec> specs;
    for (const auto& stage : stages)
    {
        const auto& simpleCommand =
            dynamic_cast<parser::ast::SimpleCommand*>(stage.get());
        if (!simpleCommand)
            continue;
        auto argv = ExpandArgv(simpleCommand->Argv());
        specs.emplace_back(BuildSpec(argv, *simpleCommand));
    }
    auto result = exec::Pipeline().Run(
        specs,
        m_state_->IsOptionEnabled("pipefail"),
        m_state_->IsJobControlEnabled() ? WaitMode::Foreground
                                        : WaitMode::UntilExit);
    m_status_ = result.status;
    if (result.state == exec::State::Stopped && m_inCompound_)
    {
        m_suspendedPgid_ = result.pgid;
        m_status_ = co_await SuspendAwaitable{.slot = m_suspendedHandle_,.statusSlot = m_resumedStatus_};
    }
    else
    {
        RecordStoppedJob(result, pipeline.SourceText());
    }
    if (pipeline.Bang())
    {
        m_status_ = (m_status_ == 0) ? 1 : 0;
    }
    co_return m_status_;
}

coro::Task Executor::Visit(parser::ast::Subshell& node)
{
    ForkRunner runner;
    m_status_ = runner.Run(
        [&]
        {
            RunToCompletion(node.GetBody());
            return m_status_;
        },
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
            auto runner = ForkRunner();
            int errCode = runner.Run(
                [&]
                {
                    setpgid(0,
                            0); // own group -> kill(-pgid) reaches it
                    m_state_->EnableJobControl(false);
                    RunToCompletion(item.node);
                    return m_status_;
                },
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