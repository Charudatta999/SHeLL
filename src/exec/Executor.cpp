#include "exec/Executor.hpp"

#include "builtins/BuiltInFunction.hpp"
#include "builtins/BuiltinDispatcher.hpp"
#include "exec/ExecHelpers.hpp"
#include "exec/ForkRunner.hpp"
#include "exec/Pipeline.hpp"
#include "parser/ast/commands/AndOr.hpp"
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
#include "shell/ShellState.hpp"

#include <fnmatch.h>
#include <memory>

namespace exec
{
Executor::Executor(std::unique_ptr<shell::ShellState>& state,
                   std::unique_ptr<builtins ::BuiltinDispatcher>& builtins)
    : m_state_(state)
    , m_builtins_(builtins)
{
}

int Executor::Run(parser::ast::AstNode& root)
{
    root.Accept(*this);
    return m_status_;
}

CommandSpec Executor::BuildSpec(const parser::ast::SimpleCommand& command) const
{
    return CommandSpec(command.Argv(), command.Redirects(), command.Assignments());
}

void Executor::Visit(parser::ast::SimpleCommand& command)
{
    if (command.Argv().empty())
    {
        auto assignments = command.Assignments();
        for (const auto& assignment : assignments)
            m_state_->SetVar(assignment.first, assignment.second);
        m_status_ = 0;
        return;
    }
    if (auto* body = m_state_->GetFunctionBody(command.Argv()[0]))
    {
        body->Accept(*this);
        return;
    }
    else if (!command.Argv().empty() && m_builtins_->IsBuiltin(command.Argv()[0]))
    {
        // auto FdVec = command.Redirects();
        auto ctx = std::make_unique<builtins::BuiltinContext>(m_state_);
        m_status_ = m_builtins_->Run(command.Argv(), ctx);
    }
    else
    {
        auto spec = BuildSpec(command);
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
        const auto& simpleCommand = dynamic_cast<parser::ast::SimpleCommand*>(stage.get());
        if (!simpleCommand)
            continue;
        specs.emplace_back(BuildSpec(*simpleCommand));
    }
    m_status_ = exec::Pipeline().Run(specs, m_state_->IsOptionEnabled("pipefail"));
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
        // #TODO: Issue filed #3 need to implement job control mechanism for background process to
        // support &
        item.node->Accept(*this);
    }
}

void Executor::Visit(parser::ast::AndOr& command)
{
    command.Lhs()->Accept(*this);
    if ((m_status_ == 0 && (command.Operator() == parser::ast::AndOr::Op::And)))
    {
        command.Rhs()->Accept(*this);
    }
    else if (m_status_ != 0 && (command.Operator() == parser::ast::AndOr::Op::Or))
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
} // namespace exec