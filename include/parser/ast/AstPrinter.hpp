#ifndef PARSER_AST_AST_PRINTER_HPP
#define PARSER_AST_AST_PRINTER_HPP

#include "parser/ast/AstVisitor.hpp"

#include <cstddef>

namespace parser::ast
{
class AstPrinter : public AstVisitor
{
public:
    AstPrinter();
    ~AstPrinter();
    AstPrinter(const AstPrinter&) = delete;
    AstPrinter& operator=(const AstPrinter&) = delete;
    AstPrinter(AstPrinter&&) = delete;
    AstPrinter& operator=(AstPrinter&&) = delete;

    void Visit(SimpleCommand& node) override;
    void Visit(Pipeline& node) override;
    void Visit(AndOr& node) override;
    void Visit(List& node) override;
    void Visit(Subshell& node) override;
    void Visit(Group& node) override;
    void Visit(Function& node) override;
    void Visit(While& node) override;
    void Visit(For& node) override;
    void Visit(If& node) override;
    void Visit(Case& node) override;
    void Visit(ArithmeticCommand& node) override;

private:
    void Indent() const;
    std::size_t m_depth_;
};
} // namespace parser::ast
#endif // PARSER_AST_AST_PRINTER_HPP