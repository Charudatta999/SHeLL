#ifndef PARSER_AST_PIPELINE_HPP
#define PARSER_AST_PIPELINE_HPP
#include "parser/ast/AstNode.hpp"

#include <memory>
#include <vector>

namespace parser::ast
{
class AstVisitor;

class Pipeline final : public AstNode
{
public:
    Pipeline(std::vector<std::unique_ptr<AstNode>> stages, bool bang)
        : m_stages_(std::move(stages))
        , m_bang_(bang)
    {
    }

    ~Pipeline() = default;
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = delete;
    Pipeline& operator=(Pipeline&&) = delete;

    void Accept(AstVisitor& visitor) override;

    [[nodiscard]]
    coro::Task Accept(ExecVisitor& visitor) override;

    [[nodiscard]]
    const std::vector<std::unique_ptr<AstNode>>& Stages() const
    {
        return m_stages_;
    }

    [[nodiscard]]
    bool Bang() const
    {
        return m_bang_;
    }

private:
    std::vector<std::unique_ptr<AstNode>> m_stages_;
    bool m_bang_;
};
} // namespace parser::ast
#endif // PARSER_AST_PIPELINE_HPP