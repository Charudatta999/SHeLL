#ifndef PARSER_AST_WHILE_HPP
#define PARSER_AST_WHILE_HPP

#include "parser/ast/AstNode.hpp"

#include <memory>

namespace parser::ast
{

class AstVisitor;

class While final : public AstNode
{
public:
    While(std::unique_ptr<AstNode> condition, std::unique_ptr<AstNode> body, bool until = false)
        : m_condition_(std::move(condition))
        , m_body_(std::move(body))
        , m_until_(until)
    {
    }

    ~While() = default;
    While(const While&) = delete;
    While& operator=(const While&) = delete;
    While(While&&) = delete;
    While& operator=(While&&) = delete;

    void Accept(AstVisitor& visitor) override;

    [[nodiscard]]
    const std::unique_ptr<AstNode>& GetCondition() const
    {
        return m_condition_;
    }

    [[nodiscard]]
    const std::unique_ptr<AstNode>& GetBody() const
    {
        return m_body_;
    }

    [[nodiscard]]
    bool IsUntil() const
    {
        return m_until_;
    }

private:
    std::unique_ptr<AstNode> m_condition_;
    std::unique_ptr<AstNode> m_body_;
    bool m_until_;
};

} // namespace parser::ast
#endif // PARSER_AST_WHILE_HPP