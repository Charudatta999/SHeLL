#include "parser/ast/AstPrinter.hpp"

// needed because AstPrinter's
// parent class only forward declares them.
#include "parser/ast/Ast.hpp"

#include <iostream>

namespace parser::ast
{
AstPrinter::AstPrinter() : m_depth_(0) {}

AstPrinter::~AstPrinter() = default;

void AstPrinter::Indent() const
{
    size_t spacesToPrint = m_depth_ * 2;
    for (size_t i = 0; i < spacesToPrint; i++)
    {
        std::cout << " ";
    }
}

void AstPrinter::Visit(SimpleCommand& node)
{
    Indent();
    std::cout << "SimpleCommand :";
    for (const auto& word : node.Argv())
    {
        std::cout << " " << word;
    }
    std::cout << "\n";
}

void AstPrinter::Visit(Pipeline& node)
{
    Indent();
    std::cout << "Pipeline" << "\n";
    ++m_depth_;
    for (const auto& stage : node.Stages())
    {
        stage->Accept(*this);
    }
    --m_depth_;
}

void AstPrinter::Visit(AndOr& node)
{
    Indent();
    std::cout << "AndOr: " << node.GetOperatorString() << "\n";
    ++m_depth_;
    node.Lhs()->Accept(*this);
    node.Rhs()->Accept(*this);
    --m_depth_;
}

void AstPrinter::Visit(List& node)
{
    Indent();
    std::cout << "List" << "\n";
    ++m_depth_;
    for (const auto& item : node.GetItems())
    {
        item.node->Accept(*this);
        if (item.background)
        {
            Indent();
            std::cout << "(background &)\n";
        }
    }
    --m_depth_;
}

void AstPrinter::Visit(Subshell& node)
{
    Indent();
    std::cout << "Subshell\n";
    ++m_depth_;
    if (node.GetBody())
        node.GetBody()->Accept(*this);
    --m_depth_;
}

void AstPrinter::Visit(Group& node)
{
    Indent();
    std::cout << "Group\n";
    ++m_depth_;
    if (node.GetBody())
        node.GetBody()->Accept(*this);
    --m_depth_;
}

void AstPrinter::Visit(Function& node)
{
    Indent();
    std::cout << "Function: " << node.GetName() << "\n";
    ++m_depth_;
    if (node.GetBody())
        node.GetBody()->Accept(*this);
    --m_depth_;
}

void AstPrinter::Visit(While& node)
{
    Indent();
    std::cout << (node.IsUntil() ? "Until\n" : "While\n");
    ++m_depth_;
    Indent();
    std::cout << "condition:\n";
    ++m_depth_;
    if (node.GetCondition())
        node.GetCondition()->Accept(*this);
    --m_depth_;
    Indent();
    std::cout << "body:\n";
    ++m_depth_;
    if (node.GetBody())
        node.GetBody()->Accept(*this);
    --m_depth_;
    --m_depth_;
}

void AstPrinter::Visit(For& node)
{
    Indent();
    std::cout << "For: " << node.GetVar() << " in";
    for (const auto& w : node.GetWords())
        std::cout << ' ' << w;
    std::cout << "\n";
    ++m_depth_;
    if (node.GetBody())
        node.GetBody()->Accept(*this);
    --m_depth_;
}

void AstPrinter::Visit(CStyleFor& node)
{
    auto expr = [](const std::unique_ptr<AstNode>& part) -> std::string
    {
        if (auto* arith =
                dynamic_cast<ArithmeticCommand*>(part.get()))
            return arith->GetExpr();
        return "";
    };

    Indent();
    std::cout << "C-style For: ((" << expr(node.GetInit()) << ";"
              << expr(node.GetCond()) << ";" << expr(node.GetUpdate())
              << "));";
    std::cout << "\n";
    ++m_depth_;
    if (node.GetBody())
        node.GetBody()->Accept(*this);
    --m_depth_;
}

void AstPrinter::Visit(Select& node)
{
    Indent();
    std::cout << "Select: " << node.GetVar() << " in";
    for (const auto& w : node.GetWords())
        std::cout << ' ' << w;
    std::cout << "\n";
    ++m_depth_;
    if (node.GetBody())
        node.GetBody()->Accept(*this);
    --m_depth_;
}

void AstPrinter::Visit(If& node)
{
    Indent();
    std::cout << "If\n";
    ++m_depth_;
    for (const auto& branch : node.GetBranches())
    {
        Indent();
        std::cout << "condition:\n";
        ++m_depth_;
        if (branch.condition)
            branch.condition->Accept(*this);
        --m_depth_;
        Indent();
        std::cout << "body:\n";
        ++m_depth_;
        if (branch.body)
            branch.body->Accept(*this);
        --m_depth_;
    }
    if (node.GetElseBody())
    {
        Indent();
        std::cout << "else:\n";
        ++m_depth_;
        node.GetElseBody()->Accept(*this);
        --m_depth_;
    }
    --m_depth_;
}

void AstPrinter::Visit(Case& node)
{
    Indent();
    std::cout << "Case: " << node.GetWord() << "\n";
    ++m_depth_;
    for (const auto& arm : node.GetArms())
    {
        Indent();
        std::cout << "pattern:";
        for (const auto& p : arm.patterns)
            std::cout << ' ' << p;
        std::cout << "\n";
        ++m_depth_;
        if (arm.body)
            arm.body->Accept(*this);
        --m_depth_;
    }
    --m_depth_;
}

void AstPrinter::Visit(ArithmeticCommand& node)
{
    Indent();
    std::cout << "Arithmetic Command\n";
    ++m_depth_;
    if (!node.GetExpr().empty())
    {
        Indent();
        std::cout << node.GetExpr() << "\n";
    }
    --m_depth_;
}

} // namespace parser::ast