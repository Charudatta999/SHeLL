#ifndef PARSER_AST_AST_VISITOR_HPP
#define PARSER_AST_AST_VISITOR_HPP

namespace parser::ast
{
class SimpleCommand;
class Pipeline;
class AndOr;
class List;
class Subshell;
class Group;
class Function;
class While;
class For;
class If;
class Case;
class ArithmeticCommand;

class AstVisitor
{
public:
    AstVisitor() = default;
    virtual ~AstVisitor() = default;
    AstVisitor(const AstVisitor&) = delete;
    AstVisitor& operator=(const AstVisitor&) = delete;
    AstVisitor(AstVisitor&&) = delete;
    AstVisitor& operator=(AstVisitor&&) = delete;

    virtual void Visit(SimpleCommand&) = 0;
    virtual void Visit(Pipeline&) = 0;
    virtual void Visit(AndOr&) = 0;
    virtual void Visit(List&) = 0;
    virtual void Visit(Subshell&) = 0;
    virtual void Visit(Group&) = 0;
    virtual void Visit(Function&) = 0;
    virtual void Visit(While&) = 0;
    virtual void Visit(For&) = 0;
    virtual void Visit(If&) = 0;
    virtual void Visit(Case&) = 0;
    virtual void Visit(ArithmeticCommand&) = 0;
};
} // namespace parser::ast
#endif // PARSER_AST_AST_VISITOR_HPP