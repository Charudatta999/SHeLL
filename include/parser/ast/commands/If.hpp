#ifndef PARSER_AST_IF_HPP
#define PARSER_AST_IF_HPP

#include "parser/ast/AstNode.hpp"

#include <memory>
#include <vector>

namespace parser::ast
{

class AstVisitor;

class If : public AstNode
{
public:
    struct Branch
    {
        std::unique_ptr<AstNode> condition;
        std::unique_ptr<AstNode> body;
    };

    If(std::vector<Branch> branches, std::unique_ptr<AstNode> elseBody = nullptr)
        : m_branches_(std::move(branches))
        , m_elseBody_(std::move(elseBody))
    {
    }

    ~If() = default;
    If(const If&) = delete;
    If& operator=(const If&) = delete;
    If(If&&) = delete;
    If& operator=(If&&) = delete;
    void Accept(AstVisitor& visitor) override;

    [[nodiscard]] const std::vector<Branch>& GetBranches() const
    {
        return m_branches_;
    }

    [[nodiscard]] const std::unique_ptr<AstNode>& GetElseBody() const
    {
        return m_elseBody_;
    }

private:
    std::vector<Branch> m_branches_;
    std::unique_ptr<AstNode> m_elseBody_;
};

} // namespace parser::ast
#endif // PARSER_AST_IF_HPP