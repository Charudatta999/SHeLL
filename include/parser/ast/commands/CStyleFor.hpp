#ifndef PARSER_AST_CSTYLEFOR_HPP
#define PARSER_AST_CSTYLEFOR_HPP

#include "parser/ast/AstNode.hpp"

#include <memory>
#include <string>

namespace parser::ast
{
class AstVisitor;

class CStyleFor final : public AstNode
{
public:
    CStyleFor(std::unique_ptr<AstNode> init,
              std::unique_ptr<AstNode> cond,
              std::unique_ptr<AstNode> update,
              std::unique_ptr<AstNode> body)
        : m_init_(std::move(init))
        , m_cond_(std::move(cond))
        , m_update_(std::move(update))
        , m_body_(std::move(body))
    {
    }

    ~CStyleFor() = default;
    CStyleFor(const CStyleFor&) = delete;
    CStyleFor& operator=(const CStyleFor&) = delete;
    CStyleFor(CStyleFor&&) = delete;
    CStyleFor& operator=(CStyleFor&&) = delete;

    void Accept(AstVisitor& visitor) override;

    [[nodiscard]]
    coro::Task Accept(ExecVisitor& visitor) override;

    [[nodiscard]]
    const std::unique_ptr<AstNode>& GetInit() const { return m_init_; }

    [[nodiscard]]
    const std::unique_ptr<AstNode>& GetCond() const { return m_cond_; }

    [[nodiscard]]
    const std::unique_ptr<AstNode>& GetUpdate() const { return m_update_; }

    [[nodiscard]]
    const std::unique_ptr<AstNode>& GetBody() const { return m_body_; }

private:
    std::unique_ptr<AstNode> m_init_;
    std::unique_ptr<AstNode> m_cond_;
    std::unique_ptr<AstNode> m_update_;
    std::unique_ptr<AstNode> m_body_;
};

} // namespace parser::ast
#endif // PARSER_AST_CSTYLEFOR_HPP
