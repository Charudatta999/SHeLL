#ifndef PARSER_AST_EXECVISITOR_HPP
#define PARSER_AST_EXECVISITOR_HPP

#include "coro/Task.hpp"

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
class CStyleFor;
class Select;
class If;
class Case;
class ArithmeticCommand;

class ExecVisitor
{
public:
    ExecVisitor() = default;
    virtual ~ExecVisitor() = default;
    ExecVisitor(const ExecVisitor&) = delete;
    ExecVisitor& operator=(const ExecVisitor&) = delete;
    ExecVisitor(ExecVisitor&&) = delete;
    ExecVisitor& operator=(ExecVisitor&&) = delete;

    virtual coro::Task Visit(SimpleCommand&) = 0;
    virtual coro::Task Visit(Pipeline&) = 0;
    virtual coro::Task Visit(AndOr&) = 0;
    virtual coro::Task Visit(List&) = 0;
    virtual coro::Task Visit(Subshell&) = 0;
    virtual coro::Task Visit(Group&) = 0;
    virtual coro::Task Visit(Function&) = 0;
    virtual coro::Task Visit(While&) = 0;
    virtual coro::Task Visit(For&) = 0;
    virtual coro::Task Visit(CStyleFor&) = 0;
    virtual coro::Task Visit(Select&) = 0;
    virtual coro::Task Visit(If&) = 0;
    virtual coro::Task Visit(Case&) = 0;
    virtual coro::Task Visit(ArithmeticCommand&) = 0;
};

} // namespace parser::ast
#endif // PARSER_AST_EXECVISITOR_HPP