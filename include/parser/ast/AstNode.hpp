#ifndef PARSER_AST_NODE_HPP
#define PARSER_AST_NODE_HPP

#include "parser/ast/Redirect.hpp"
#include "coro/Task.hpp"

#include <string>
#include <vector>
namespace parser::ast
{

// Forward Declare
class AstVisitor;
class ExecVisitor;

class AstNode
{
public:
    AstNode() = default;
    explicit AstNode(std::vector<Redirect> redirects)
    : m_redirects_(std::move(redirects)) {}
    virtual ~AstNode() = default;
    AstNode(const AstNode& astNode) = delete;
    AstNode& operator=(const AstNode& astNode) = delete;
    AstNode(AstNode&& astnode) = delete;
    AstNode& operator=(AstNode&& astNode) = delete;

    virtual void Accept(AstVisitor& visitor) = 0;

    virtual coro::Task Accept(ExecVisitor&) = 0;

    // Original source text of the command this node represents, set by
    // the parser from the consumed token range. Used for job display
    // (jobs/fg/announcements) instead of reconstructing from argv.
    void SetSourceText(std::string text)
    {
        m_sourceText_ = std::move(text);
    }
    [[nodiscard]] const std::string& SourceText() const
    {
        return m_sourceText_;
    }
    const std::vector<Redirect>& Redirects() const
    {
        return m_redirects_;
    }
protected:
    std::vector<Redirect> m_redirects_;
private:
    std::string m_sourceText_;
};
} // namespace parser::ast
#endif // PARSER_AST_NODE_HPP