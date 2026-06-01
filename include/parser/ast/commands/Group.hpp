#ifndef PARSER_AST_GROUP_HPP
#define PARSER_AST_GROUP_HPP

#include "parser/ast/AstNode.hpp"

#include <memory>
namespace parser::ast
{
class AstVisitor;
class Group : public AstNode
{
public:
    Group(std::unique_ptr<AstNode> body)  : m_body_(std::move(body)) {};
    ~Group() = default;
    Group(const Group&) = delete;
    Group& operator=(const Group&) = delete;
    Group(Group&&) = delete;
    Group& operator=(Group&&) = delete;

    void Accept(AstVisitor& visitor) override;
    [[nodiscard]]
    const std::unique_ptr<AstNode>& GetBody() const { return m_body_; }

private:
    std::unique_ptr<AstNode> m_body_;

};

} // namespace parser::ast
#endif // PARSER_AST_GROUP_HPP