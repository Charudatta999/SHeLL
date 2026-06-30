#include "parser/ast/Ast.hpp"

#include "parser/ast/AstVisitor.hpp"
#include "parser/ast/ExecVisitor.hpp"

namespace parser::ast
{

void SimpleCommand::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void Pipeline::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void AndOr::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void List::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void Subshell::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void Group::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void Function::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void While::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void For::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void CStyleFor::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void If::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void Case::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

void ArithmeticCommand::Accept(AstVisitor& visitor)
{
    visitor.Visit(*this);
}

coro::Task SimpleCommand::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task Pipeline::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task AndOr::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task List::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task Subshell::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task Group::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task Function::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task While::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task For::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task CStyleFor::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task If::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task Case::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

coro::Task ArithmeticCommand::Accept(ExecVisitor& visitor)
{
    return visitor.Visit(*this);
}

} // namespace parser::ast