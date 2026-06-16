#ifndef PARSER_AST_CASE_HPP
#define PARSER_AST_CASE_HPP

#include "parser/ast/AstNode.hpp"

#include <memory>
#include <string>
#include <vector>

namespace parser::ast
{
class AstVisitor;

class Case final : public AstNode
{
public:
    struct Arm
    {
        std::vector<std::string> patterns;
        std::unique_ptr<AstNode> body;
    };

    Case(std::string word, std::vector<Arm> arms)
        : m_word_(std::move(word))
        , m_arms_(std::move(arms))
    {
    }

    ~Case() = default;
    Case(const Case&) = delete;
    Case& operator=(const Case&) = delete;
    Case(Case&&) = delete;
    Case& operator=(Case&&) = delete;

    void Accept(AstVisitor& visitor) override;

    [[nodiscard]]
    coro::Task Accept(ExecVisitor& visitor) override;

    [[nodiscard]]
    const std::string& GetWord() const
    {
        return m_word_;
    }

    [[nodiscard]]
    const std::vector<Arm>& GetArms() const
    {
        return m_arms_;
    }

private:
    std::string m_word_;
    std::vector<Arm> m_arms_;
};

} // namespace parser::ast
#endif // PARSER_AST_CASE_HPP