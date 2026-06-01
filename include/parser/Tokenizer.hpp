#ifndef PARSER_TOKENIZER_HPP
#define PARSER_TOKENIZER_HPP

#include "parser/Token.hpp"

#include <string>
#include <vector>

namespace parser
{

class Tokenizer
{
public:
    Tokenizer(const std::string& command);
    ~Tokenizer() = default;
    Tokenizer(const Tokenizer&) = delete;
    Tokenizer& operator=(const Tokenizer&) = delete;
    Tokenizer(Tokenizer&&) = delete;
    Tokenizer& operator=(Tokenizer&&) = delete;

    [[nodiscard]]
    std::vector<Token> Tokenize();

private:
    [[nodiscard]]
    Token ReadWord();
    [[nodiscard]]
    Token ReadOperator();
    [[nodiscard]]
    Token ReadSingleQuoted();
    [[nodiscard]]
    Token ReadDoubleQuoted();
    [[nodiscard]]
    char Peek(int ahead = 0) const;

    char Advance();
    [[nodiscard]]
    bool AtEnd() const;
    [[nodiscard]]
    bool IsOperatorStart(char chr) const;
    [[nodiscard]]
    bool IsWordChar(char chr) const;
    [[nodiscard]]
    TokenType ResolveKeyword(const std::string& word);
    void SkipWhitespace();

    std::string m_command_;
    std::size_t m_pos_ = 0;
    std::size_t m_line_ = 1;
    std::size_t m_col_ = 1;
};
} // namespace parser

#endif // PARSER_TOKENIZER_HPP
