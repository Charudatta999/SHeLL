#ifndef PARSER_TOKEN_HPP
#define PARSER_TOKEN_HPP

#include <string>
#include <string_view>

namespace parser
{

// ─── Token Types ─────────────────────────────────────────────────────────────
// One type per distinct syntactic meaning.
// Mirrors ZSH lex.c lextok enum + redirect kinds from zsh.h.
// Keywords are given their own types; the parser decides when they are
// keywords vs plain words based on position.
// ─────────────────────────────────────────────────────────────────────────────
enum class TokenType
{
    // ── Words ─────────────────────────────────────────────────────────────
    Word,         // unquoted word: command name, argument, filename
    SingleQuoted, // '...' — literal, no expansion
    DoubleQuoted, // "..." — allows $, ``, \

    // ── Pipe family ───────────────────────────────────────────────────────
    Pipe,     // |    stdout of lhs → stdin of rhs
    PipeBoth, // |&   stdout+stderr of lhs → stdin of rhs

    // ── Logical operators ─────────────────────────────────────────────────
    And, // &&   run rhs only if lhs exits 0
    Or,  // ||   run rhs only if lhs exits non-0

    // ── Statement separators ──────────────────────────────────────────────
    Semi,          // ;    sequential
    DoubleSemi,    // ;;   case arm terminator
    SemiAmp,       // ;&   case fallthrough (no re-test)
    DoubleSemiAmp, // ;;& case fallthrough (re-test)
    Background,    // &    run in background (don't wait)
    Newline,       // \n   terminates a command (significant)

    // ── Redirects: input ──────────────────────────────────────────────────
    RedirIn,        // <    stdin from file           (default fd 0)
    RedirReadWrite, // <>   open file read+write      (default fd 0)
    HereDoc,        // <<   here-document             (default fd 0)
    HereDocDash,    // <<-  here-document, strip tabs (default fd 0)
    HereString,     // <<<  here-string               (default fd 0)
    DupIn,          // <&   duplicate input fd        (default fd 0)

    // ── Redirects: output ─────────────────────────────────────────────────
    RedirOut,        // >    stdout to file            (default fd 1)
    RedirAppend,     // >>   stdout append             (default fd 1)
    RedirClobber,    // >|   force overwrite (noclobber bypass)
    DupOut,          // >&   duplicate output fd       (default fd 1)
    RedirBoth,       // &>   stdout+stderr to file     (default fd 1)
    RedirBothAppend, // &>>  stdout+stderr append      (default fd 1)

    // ── Grouping ──────────────────────────────────────────────────────────
    LParen,    // (    subshell open
    RParen,    // )    subshell close
    LBrace,    // {    group command open
    RBrace,    // }    group command close
    DLParen,   // ((   arithmetic compound command
    DRParen,   // ))   arithmetic compound close
    DLBracket, // [[   extended test open
    DRBracket, // ]]   extended test close

    // ── Keywords ──────────────────────────────────────────────────────────
    // Only meaningful at command position; plain words everywhere else.
    If,
    Then,
    Elif,
    Else,
    Fi,
    While,
    Until,
    Do,
    Done,
    For,
    Foreach,
    End,
    In,
    Case,
    Esac,
    Select,
    Function,
    Time,
    Bang, // !   negate pipeline exit status / history expansion

    // ── Structural ────────────────────────────────────────────────────────
    Eof, // end of input
};

// ─── Token ───────────────────────────────────────────────────────────────────
// Mirrors ZSH's (tok, tokstr, tokfd) per-token, encapsulated as a struct.
//
//   type   — always set
//   value  — set for: Word, SingleQuoted, DoubleQuoted, DupIn/DupOut target,
//             HereDoc delimiter, HereString body
//   fd     — set for redirect tokens when an explicit fd prefix was written
//             (e.g. 2> sets fd=2). -1 means "use the default for this type".
//   line   — 1-based source line (for error messages)
//   col    — 1-based source column
// ─────────────────────────────────────────────────────────────────────────────
struct Token
{
    TokenType type;
    std::string value;
    int fd = -1;
    size_t line = 0;
    size_t col = 0;
};

// ─── Helper declarations (implemented in Token.cpp) ──────────────────────────
std::string_view tokenTypeName(TokenType type);

} // namespace parser

#endif // PARSER_TOKEN_HPP
