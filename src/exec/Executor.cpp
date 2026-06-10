#include "exec/Executor.hpp"

#include "arithmetic/ArithmeticEngine.hpp"
#include "arithmetic/ArithmeticException.hpp"
#include "builtins/BuiltInFunction.hpp"
#include "builtins/BuiltinDispatcher.hpp"
#include "exec/ExecHelpers.hpp"
#include "exec/ForkRunner.hpp"
#include "exec/Pipeline.hpp"
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
#include "shell/ShellArithmeticVars.hpp"
#include "shell/ShellState.hpp"
#include "shell/expander/Expander.hpp"

#include <fnmatch.h>
#include <memory>

namespace exec
{
Executor::Executor(
    std::unique_ptr<shell::ShellState>& state,
    std::unique_ptr<builtins ::BuiltinDispatcher>& builtins)
    : m_state_(state)
    , m_builtins_(builtins)
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
    return CommandSpec(argv, command.Redirects(), command.Assignments());
}

std::vector<std::string>
Executor::ExpandArgv(const std::vector<std::string>& argv)
{
    std::vector<std::string> out;
    for (const auto& word : argv)
    {
        auto pieces = shell::expander::Expand(word, m_state_);
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
                shell::expander::Expand(assignment.second, m_state_).front());
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
        m_status_ = pipeline.Run({spec});
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
    m_status_ =
        exec::Pipeline().Run(specs,
                             m_state_->IsOptionEnabled("pipefail"));
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
        });
}

void Executor::Visit(parser::ast::List& list)
{
    const auto& items = list.GetItems();
    for (const auto& item : items)
    {
        // #TODO: Issue filed #3 need to implement job control
        // mechanism for background process to support &
        item.node->Accept(*this);
    }
}

void Executor::Visit(parser::ast::AndOr& command)
{
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
    group.GetBody()->Accept(*this);
}

void Executor::Visit(parser::ast::While& condi)
{
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
    const auto& words = loop.GetWords();
    for (const auto& word : words)
    {
        m_state_->SetVar(loop.GetVar(), word);
        loop.GetBody()->Accept(*this);
    }
}

void Executor::Visit(parser::ast::If& condi)
{
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
            m_status_ = (result != 0 ) ? 0 :1;
    }
    catch (const arithmetic::ArithmeticException& ex)
    {
        m_status_ = 1;
        // to do log the error when logger is intergrated
    }
}
} // namespace exec