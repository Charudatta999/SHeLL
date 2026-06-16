#ifndef PARSER_PARSER_HPP
#define PARSER_PARSER_HPP

#include "parser/Token.hpp"
#include "parser/ast/AstNode.hpp" //Light weight header so it can be included.

#include <memory>
#include <vector>

namespace parser
{

class Parser
{
public:
    explicit Parser(std::vector<Token> tokens);

    [[nodiscard]]
    std::unique_ptr<ast::AstNode> Parse();

private:
    [[nodiscard]]
    const Token& Peek(int offset = 0) const;
    const Token& Advance();
    [[nodiscard]]
    bool Check(TokenType type) const;
    [[nodiscard]]
    bool Match(TokenType type); // consume if matches

    const Token& Expect(TokenType type, const char* context);

    [[nodiscard]]
    bool AtEnd() const;
    void SkipNewlines();

    [[nodiscard]]
    bool IsAssignment(const std::string&) const;

    [[nodiscard]]
    bool IsListTerminator(TokenType type) const;

    [[nodiscard]]
    bool IsWordLike(TokenType type) const;

    std::unique_ptr<ast::AstNode> ParseList();
    std::unique_ptr<ast::AstNode> ExpectList(const char* context);
    std::unique_ptr<ast::AstNode> ParseAndOr();
    std::unique_ptr<ast::AstNode> ParsePipeline();
    std::unique_ptr<ast::AstNode> ParseCommand();
    std::unique_ptr<ast::AstNode> ParseSimpleCommand();
    std::unique_ptr<ast::AstNode> ParseSubshell();
    std::unique_ptr<ast::AstNode> ParseGroup();
    std::unique_ptr<ast::AstNode> ParseFunction(std::string name);
    std::unique_ptr<ast::AstNode> ParseIf();
    std::unique_ptr<ast::AstNode> ParseWhile();
    std::unique_ptr<ast::AstNode> ParseFor();
    std::unique_ptr<ast::AstNode> ParseCase();
    std::unique_ptr<ast::AstNode> ParseArithmeticCommand();

    [[nodiscard]] std::string SourceTextFrom(std::size_t start) const;

    std::vector<Token> m_tokens_;
    std::size_t m_pos_ = 0;
};

} // namespace parser
#endif // PARSER_PARSER_HPP