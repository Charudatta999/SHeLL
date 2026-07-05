#ifndef PARSER_AST_SELECT_HPP
#define PARSER_AST_SELECT_HPP

#include "parser/ast/AstNode.hpp"

#include <memory>
#include <string>
#include <vector>

namespace parser::ast
{
class AstVisitor;

class Select final : public AstNode
{
public:
    Select(std::string var,
           std::vector<std::string> words,
           std::unique_ptr<AstNode> body)
        : m_var_(std::move(var))
        , m_words_(std::move(words))
        , m_body_(std::move(body))
    {
    }

    ~Select() = default;
    Select(const Select&) = delete;
    Select& operator=(const Select&) = delete;
    Select(Select&&) = delete;
    Select& operator=(Select&&) = delete;

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
#endif // PARSER_AST_SELECT_HPP
