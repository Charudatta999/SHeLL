#ifndef PARSER_AST_NODE_HPP
#define PARSER_AST_NODE_HPP

#include <cstddef>
#include <memory>

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

};
} // namespace parser::ast
#endif // PARSER_AST_NODE_HPP