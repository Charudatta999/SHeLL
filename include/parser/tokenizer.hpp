#ifndef PARSER_TOKENIZER_HPP
#define PARSER_TOKENIZER_HPP

#include "parser/Token.hpp"

#include <string>
#include <vector>

namespace parser
{

// ─── Tokenizer ────────────────────────────────────────────────────────────────
// Converts a shell input string into a flat vector of tokens.
//
// Design:
//   - Single-pass, left-to-right
//   - Greedy longest-match for operators (>> before >, || before |, etc.)
//   - Fd-prefixed redirects absorbed into one token (2> → RedirOut{fd=2})
//   - Keywords recognized by value; emitted as distinct token types
//   - Quoted strings stored raw; expansion is the executor's job
//   - Comments (#) consumed silently
//   - Newlines emitted as Newline tokens (significant in the grammar)
// ─────────────────────────────────────────────────────────────────────────────
class Tokenizer
{
public:
    explicit Tokenizer(std::string input);

    // Lex the entire input and return a token vector ending with Eof.
    std::vector<Token> tokenize();

private:
    std::string m_input;
    size_t      m_pos  = 0;
    int         m_line = 1;
    int         m_col  = 1;

    // ── Character navigation ──────────────────────────────────────────────
    char   peek(int offset = 0) const;
    char   advance();
    bool   atEnd() const;
    void   skipWhitespace();  // skips spaces and tabs only — not newlines

    // ── Token readers ─────────────────────────────────────────────────────
    Token readWord();
    Token readSingleQuoted();
    Token readDoubleQuoted();

    // ── Helpers ───────────────────────────────────────────────────────────
    bool isOperatorStart(char c) const;
    bool isWordChar(char c) const;

    // Returns the keyword token type for a word, or TokenType::Word if not a keyword.
    static TokenType resolveKeyword(const std::string& word);
};

} // namespace parser

#endif // PARSER_TOKENIZER_HPP
