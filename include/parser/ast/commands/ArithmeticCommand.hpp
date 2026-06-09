#ifndef PARSER_AST_COMMANDS_ARITHMETIC_COMMAND_HPP
#define PARSER_AST_COMMANDS_ARITHMETIC_COMMAND_HPP

#include "parser/ast/AstNode.hpp"
#include <string>
#include <utility>
namespace parser::ast
{
class AstVisitor;

class ArithmeticCommand  final: public AstNode
{
public:
    ArithmeticCommand(std::string expr): m_expr_(std::move(expr)) {}
    ~ArithmeticCommand() = default;
    ArithmeticCommand(const ArithmeticCommand&) = delete;
    ArithmeticCommand& operator=(const ArithmeticCommand&) = delete;
    ArithmeticCommand(ArithmeticCommand&&) = delete;
    ArithmeticCommand& operator=(ArithmeticCommand&&) = delete;

    void Accept(AstVisitor& visitor) override;
    [[nodiscard]]
    const std::string& GetExpr() const
    {
        return m_expr_;
    }

private:
    std::string m_expr_;
};

} // namespace parser::ast
#endif // PARSER_AST_COMMANDS_ARITHMETI_CCOMMAND_HPP