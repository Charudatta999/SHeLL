#ifndef PARSER_AST_SUBSHELL_HPP
#define PARSER_AST_SUBSHELL_HPP

#include "parser/ast/AstNode.hpp"

#include <memory>

namespace parser::ast
{
class AstVisitor;

class Subshell : public AstNode
{
public:
    Subshell(std::unique_ptr<AstNode> body)  : m_body_(std::move(body)) {}
    ~Subshell() = default;
    Subshell(const Subshell&) = delete;
    Subshell& operator=(const Subshell&) = delete;
    Subshell(Subshell&&) = delete;
    Subshell& operator=(Subshell&&) = delete;

    void Accept(AstVisitor& visitor) override;
    [[nodiscard]]
    const std::unique_ptr<AstNode>& GetBody() const { return m_body_; }

private:
    std::unique_ptr<AstNode> m_body_;
};

} // namespace parser::ast
#endif // PARSER_AST_SUBSHELL_HPP