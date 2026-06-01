#ifndef PARSER_AST_AND_OR_HPP
#define PARSER_AST_AND_OR_HPP

#include "parser/ast/AstNode.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace parser::ast
{
class AstVisitor;

class AndOr : public AstNode
{
  public:
    enum class Op : uint8_t
    {
        And,
        Or
    };

    AndOr(std::unique_ptr<AstNode> lhs, Op optr, std::unique_ptr<AstNode> rhs)
        : m_lhs_(std::move(lhs))
        , m_optr_(optr)
        , m_rhs_(std::move(rhs))
    {
    }

    ~AndOr() = default;
    AndOr(const AndOr&) = delete;
    AndOr& operator=(const AndOr&) = delete;
    AndOr(AndOr&&) = delete;
    AndOr& operator=(AndOr&&) = delete;

    void Accept(AstVisitor& visitor) override;

    [[nodiscard]]
    const std::unique_ptr<AstNode>& Lhs() const
    {
        return m_lhs_;
    }

    [[nodiscard]]
    const std::unique_ptr<AstNode>& Rhs() const
    {
        return m_rhs_;
    }

    [[nodiscard]]
    Op Operator() const
    {
        return m_optr_;
    }

    [[nodiscard]]
    std::string GetOperatorString() const
    {
        return (m_optr_ == Op::And) ? "&&" : "||";
    }

  private:
    std::unique_ptr<AstNode> m_lhs_;
    Op m_optr_;
    std::unique_ptr<AstNode> m_rhs_;
};

} // namespace parser::ast
#endif // PARSER_AST_ANDOR_HPP