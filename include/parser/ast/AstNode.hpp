#ifndef PARSER_AST_NODE_HPP
#define PARSER_AST_NODE_HPP

#include <string>

namespace parser::ast
{

// Forward Declare
class AstVisitor;

class AstNode
{
public:
    AstNode() = default;
    virtual ~AstNode() = default;
    AstNode(const AstNode& astNode) = delete;
    AstNode& operator=(const AstNode& astNode) = delete;
    AstNode(AstNode&& astnode) = delete;
    AstNode& operator=(AstNode&& astNode) = delete;

    virtual void Accept(AstVisitor& visitor) = 0;

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

private:
    std::string m_sourceText_;
};
} // namespace parser::ast
#endif // PARSER_AST_NODE_HPP