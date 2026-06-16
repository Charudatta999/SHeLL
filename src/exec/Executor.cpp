#include "exec/Executor.hpp"

#include "arithmetic/ArithmeticEngine.hpp"
#include "arithmetic/ArithmeticException.hpp"
#include "builtins/BuiltInFunction.hpp"
#include "builtins/BuiltinDispatcher.hpp"
#include "exec/ExecHelpers.hpp"
#include "exec/ForkRunner.hpp"
#include "exec/Pipeline.hpp"
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
// Sets a flag true for the duration of a scope, restores on exit.
// Marks "we are inside a compound command" while a compound node is
// being traversed (nests correctly).
struct CompoundScope
{
    bool& flag;
    bool prev;
    explicit CompoundScope(bool& target) : flag(target), prev(target)
    {
        flag = true;
    }
    ~CompoundScope() { flag = prev; }
    CompoundScope(const CompoundScope&) = delete;
    CompoundScope& operator=(const CompoundScope&) = delete;
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
{
}

int Executor::Run(const std::unique_ptr<parser::ast::AstNode>& root)
{
    root->Accept(*this);
    return m_status_;
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

    // A stop inside a compound (&&, list, loop, if) can't be suspended
    // as one job — the continuation lives on the shell's call stack, not
    // in any process. Rather than record a half-job and silently drop
    // the rest, resume the process and run it to completion.
    if (m_inCompound_)
    {
        kill(-result.pgid, SIGCONT);
        exec::WaitStatus done(result.pgid, exec::WaitMode::UntilExit);
        m_status_ = done.Exited()     ? done.ExitCode()
                    : done.Signaled() ? SIGNAL_EXIT_BASE + done.GetSignal()
                                      : INVALID_STATUS;
        Announce("shell: cannot suspend a compound command; resumed\n");
        return;
    }

    auto jobID = m_state_->GetJobs()->Add(
        result.pgid, commandText, shell::JobTable::State::Stopped);
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

void Executor::Visit(parser::ast::SimpleCommand& command)
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
        return;
    }
    if (auto* body = m_state_->GetFunctionBody(argv[0]))
    {
        body->Accept(*this);
        return;
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
        RecordStoppedJob(result, command.SourceText());
    }
}

void Executor::Visit(parser::ast::Pipeline& pipeline)
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
    RecordStoppedJob(result, pipeline.SourceText());
    if (pipeline.Bang())
        m_status_ = (m_status_ == 0) ? 1 : 0;
}

void Executor::Visit(parser::ast::Subshell& node)
{
    ForkRunner runner;
    m_status_ = runner.Run(
        [&]
        {
            node.GetBody()->Accept(*this);
            return m_status_;
        },
        m_state_->IsJobControlEnabled() ? WaitMode::Foreground
                                        : WaitMode::Poll);
}

void Executor::Visit(parser::ast::List& list)
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
                    setpgid(0, 0); // own group -> kill(-pgid) reaches it
                    m_state_->EnableJobControl(false);
                    item.node->Accept(*this);
                    return m_status_;
                }, WaitMode::Poll);
            if (errCode == PROCESS_RUNNING)
            {
                pid_t pid = runner.Pid();
                if (pid > 0)
                {
                    setpgid(pid, pid); // parent half of the race-guarded setpgid
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
            item.node->Accept(*this);
        }
    }
}

void Executor::Visit(parser::ast::AndOr& command)
{
    CompoundScope scope(m_inCompound_);
    command.Lhs()->Accept(*this);
    if ((m_status_ == 0 &&
         (command.Operator() == parser::ast::AndOr::Op::And)))
    {
        command.Rhs()->Accept(*this);
    }
    else if (m_status_ != 0 &&
             (command.Operator() == parser::ast::AndOr::Op::Or))
    {
        command.Rhs()->Accept(*this);
    }
}

void Executor::Visit(parser::ast::Group& group)
{
    CompoundScope scope(m_inCompound_);
    group.GetBody()->Accept(*this);
}

void Executor::Visit(parser::ast::While& condi)
{
    CompoundScope scope(m_inCompound_);
    while (true)
    {
        condi.GetCondition()->Accept(*this);
        bool keepGoing = (m_status_ == 0);
        if (condi.IsUntil())
        {
            keepGoing = !keepGoing;
        }
        if (!keepGoing)
        {
            break;
        }
        condi.GetBody()->Accept(*this);
    }
}

void Executor::Visit(parser::ast::For& loop)
{
    CompoundScope scope(m_inCompound_);
    const auto& words = loop.GetWords();
    for (const auto& word : words)
    {
        m_state_->SetVar(loop.GetVar(), word);
        loop.GetBody()->Accept(*this);
    }
}

void Executor::Visit(parser::ast::If& condi)
{
    CompoundScope scope(m_inCompound_);
    const auto& branches = condi.GetBranches();
    for (const auto& branch : branches)
    {
        branch.condition->Accept(*this);
        if (m_status_ == 0)
        {
            branch.body->Accept(*this);
            return;
        }
    }
    if (condi.GetElseBody())
    {
        condi.GetElseBody()->Accept(*this);
    }
}

void Executor::Visit(parser::ast::Case& case_)
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
                    arm.body->Accept(*this);
                return;
            }
        }
    }
}

void Executor::Visit(parser::ast::Function& node)
{
    m_state_->AddFunction(node.GetName(), node.ReleaseBody());
    m_status_ = 0;
}

void Executor::Visit(parser::ast::ArithmeticCommand& node)
{
    shell::ShellArithmeticVars adapter(m_state_);
    try
    {
        auto result =
            arithmetic::engine::Evaluate(node.GetExpr(), adapter);
        m_status_ = (result != 0) ? 0 : 1;
    }
    catch (const arithmetic::ArithmeticException& ex)
    {
        m_status_ = 1;
        // to do log the error when logger is intergrated
    }
}
} // namespace exec