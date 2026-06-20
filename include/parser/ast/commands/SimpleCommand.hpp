#ifndef PARSER_AST_SIMPLE_COMMAND_HPP
#define PARSER_AST_SIMPLE_COMMAND_HPP

#include "parser/ast/AstNode.hpp"
#include "parser/ast/Redirect.hpp"

#include <string>
#include <utility>
#include <vector>

namespace parser::ast
{
class SimpleCommand final : public AstNode
{
public:
    SimpleCommand(
        const std::vector<std::string>& argv,
        const std::vector<Redirect>& redirects,
        const std::vector<std::pair<std::string, std::string>>&
            assignments)
        : AstNode(redirects)
        , m_argv_(argv)
        , m_assignments_(assignments)
    {
    }

    void Accept(AstVisitor& visitor) override; // match base casing

    [[nodiscard]]
    coro::Task Accept(ExecVisitor& visitor) override;

    [[nodiscard]]
    const std::vector<std::string>& Argv() const
    {
        return m_argv_;
    }

    [[nodiscard]]
    const std::vector<Redirect>& Redirects() const
    {
        return m_redirects_;
    }

    [[nodiscard]]
    const std::vector<std::pair<std::string, std::string>>&
    Assignments() const
    {
        return m_assignments_;
    }

private:
    std::vector<std::string> m_argv_;
    std::vector<std::pair<std::string, std::string>> m_assignments_;
};
} // namespace parser::ast
#endif // PARSER_AST_SIMPLE_COMMAND_HPP