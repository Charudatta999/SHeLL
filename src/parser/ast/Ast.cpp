#include "parser/ast/Ast.hpp"
#include "parser/ast/AstVisitor.hpp"


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

} // namespace parser::ast