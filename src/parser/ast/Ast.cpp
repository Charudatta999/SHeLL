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

void Subshell::Accept(AstVisitor& v)
{
    v.Visit(*this);
}

void Group::Accept(AstVisitor& v)
{
    v.Visit(*this);
}

void Function::Accept(AstVisitor& v)
{
    v.Visit(*this);
}

void While::Accept(AstVisitor& v)
{
    v.Visit(*this);
}

void For::Accept(AstVisitor& v)
{
    v.Visit(*this);
}

void If::Accept(AstVisitor& v)
{
    v.Visit(*this);
}

void Case::Accept(AstVisitor& v)
{
    v.Visit(*this);
}

} // namespace parser::ast