#ifndef PARSER_PARSER_HPP
#define PARSER_PARSER_HPP

#include "parser/Token.hpp"
#include "parser/Command.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace parser
{

// ─── ParseError ───────────────────────────────────────────────────────────────
class ParseError : public std::runtime_error
{
public:
    ParseError(const std::string& msg, int line, int col)
        : std::runtime_error(msg + " (line " + std::to_string(line) +
                             ", col " + std::to_string(col) + ")")
        , m_line(line), m_col(col)
    {}

    int line() const { return m_line; }
    int col()  const { return m_col;  }

private:
    int m_line;
    int m_col;
};

// ─── Parser ───────────────────────────────────────────────────────────────────
// Recursive-descent parser. Consumes the token vector produced by Tokenizer
// and builds an AST rooted at a ListNode (or a simpler node for single items).
//
// Grammar hierarchy (lowest precedence first):
//   parseList()        → items separated by ; & \n
//   parseAndOr()       → && ||
//   parsePipeline()    → | |&
//   parseCommand()     → SimpleCommand | CompoundCommand
//   parseSimpleCommand / parseIf / parseWhile / parseFor / parseCase / ...
// ─────────────────────────────────────────────────────────────────────────────
class Parser
{
public:
    explicit Parser(std::vector<Token> tokens);

    // Parse the full token stream. Returns a ListNode (possibly single-item).
    // Throws ParseError on syntax errors.
    AstNodePtr parse();

private:
    std::vector<Token> m_tokens;
    size_t             m_pos = 0;

    // ── Token stream helpers ──────────────────────────────────────────────
    const Token& peek(int offset = 0) const;
    const Token& advance();
    bool         check(TokenType type) const;
    bool         match(TokenType type);           // consume if matches, return true
    const Token& expect(TokenType type, const char* context);
    bool         atEnd() const;
    void         skipNewlines();

    // ── Classification helpers ────────────────────────────────────────────
    bool isWordLike(TokenType t) const;           // Word, SingleQuoted, DoubleQuoted, keywords-as-arg
    bool isRedirectToken(TokenType t) const;
    bool isPipelineTerminator(TokenType t) const; // stops simple-cmd word collection
    bool isListTerminator(TokenType t) const;     // stops list parsing (RParen, RBrace, Eof)

    // Extract word-string from a word-like token (keyword value stored in value field).
    std::string tokenWord(const Token& tok) const;

    // ── Redirect parsing ──────────────────────────────────────────────────
    Redirect parseOneRedirect();
    void     parseRedirects(std::vector<Redirect>& out);

    // ── Grammar rules ─────────────────────────────────────────────────────
    AstNodePtr parseList();
    AstNodePtr parseAndOr();
    AstNodePtr parsePipeline();
    AstNodePtr parseCommand();

    AstNodePtr parseSimpleCommand();
    AstNodePtr parseIf();
    AstNodePtr parseWhile();      // handles both while and until
    AstNodePtr parseFor();
    AstNodePtr parseCase();
    AstNodePtr parseSubshell();   // ( list )
    AstNodePtr parseGroup();      // { list ; }
    AstNodePtr parseFunction(const std::string& name);  // name() compound

    // Parse a compound command's body (the list between do/then/{ and done/fi/})
    AstNodePtr parseCompoundBody();
};

} // namespace parser

#endif // PARSER_PARSER_HPP
