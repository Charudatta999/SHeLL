#ifndef PARSER_AST_FOR_HPP
#define PARSER_AST_FOR_HPP

#include "parser/ast/AstNode.hpp"

#include <memory>
#include <string>
#include <vector>

namespace parser::ast
{
class AstVisitor;

class For final : public AstNode
{
public:
    For(std::string var,
        std::vector<std::string> words,
        std::unique_ptr<AstNode> body)
        : m_var_(std::move(var))
        , m_words_(std::move(words))
        , m_body_(std::move(body))
    {
    }

    ~For() = default;
    For(const For&) = delete;
    For& operator=(const For&) = delete;
    For(For&&) = delete;
    For& operator=(For&&) = delete;

    void Accept(AstVisitor& visitor) override;

    [[nodiscard]]
    coro::Task Accept(ExecVisitor& visitor) override;

    [[nodiscard]]
    const std::string& GetVar() const
    {
        return m_var_;
    }

    [[nodiscard]]
    const std::vector<std::string>& GetWords() const
    {
        return m_words_;
    }

    [[nodiscard]]
    const std::unique_ptr<AstNode>& GetBody() const
    {
        return m_body_;
    }

private:
    std::string m_var_;
    std::vector<std::string> m_words_;
    std::unique_ptr<AstNode> m_body_;
};

} // namespace parser::ast
#endif // PARSER_AST_FOR_HPP