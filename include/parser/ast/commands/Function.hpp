#ifndef PARSER_AST_FUNCTION_HPP
#define PARSER_AST_FUNCTION_HPP

#include "parser/ast/AstNode.hpp"

#include <memory>
#include <string>

namespace parser::ast
{
class AstVisitor;

class Function final : public AstNode
{
public:
    Function(std::string name, std::unique_ptr<AstNode> body)
        : m_name_(std::move(name))
        , m_body_(std::move(body))
    {
    }

    ~Function() = default;
    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;
    Function(Function&&) = delete;
    Function& operator=(Function&&) = delete;

    void Accept(AstVisitor& visitor) override;

    [[nodiscard]]
    coro::Task Accept(ExecVisitor& visitor) override;

    [[nodiscard]]
    const std::string& GetName() const
    {
        return m_name_;
    }

    [[nodiscard]]
    const std::unique_ptr<AstNode>& GetBody() const
    {
        return m_body_;
    }

    std::unique_ptr<AstNode> ReleaseBody() { return std::move(m_body_); }

private:
    std::string m_name_;
    std::unique_ptr<AstNode> m_body_;
};
} // namespace parser::ast
#endif // PARSER_AST_FUNCTION_HPP