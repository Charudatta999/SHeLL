#ifndef PARSER_AST_LIST_HPP
#define PARSER_AST_LIST_HPP

#include "parser/ast/AstNode.hpp"

#include <memory>
#include <vector>

namespace parser::ast
{
class AstVisitor;


class List final : public AstNode
{
public:
    struct Item
    {
        std::unique_ptr<AstNode> node;
        bool background = false;
    };
    List(std::vector<Item> items)
        : m_items_{std::move(items)}
    {
    }

    ~List() = default;
    List(const List&) = delete;
    List& operator=(const List&) = delete;
    List(List&&) = delete;
    List& operator=(List&&) = delete;

    void Accept(AstVisitor& visitor) override;

    [[nodiscard]]
    const std::vector<Item>& GetItems() const
    {
        return m_items_;
    };

private:
    std::vector<Item> m_items_;
};

} // namespace parser::ast
#endif // PARSER_AST_LIST_HPP