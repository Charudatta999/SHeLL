#include "parser/Tokenizer.hpp"

#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace parser
{

// ─── Constructor ──────────────────────────────────────────────────────────────

Tokenizer::Tokenizer(std::string input)
    : m_input(std::move(input))
{}

// ─── Character navigation ─────────────────────────────────────────────────────

char Tokenizer::peek(int offset) const
{
    size_t idx = m_pos + static_cast<size_t>(offset);
    if (idx >= m_input.size()) return '\0';
    return m_input[idx];
}

char Tokenizer::advance()
{
    char c = m_input[m_pos++];
    if (c == '\n') { ++m_line; m_col = 1; }
    else           { ++m_col; }
    return c;
}

bool Tokenizer::atEnd() const
{
    return m_pos >= m_input.size();
}

void Tokenizer::skipWhitespace()
{
    while (!atEnd() && (peek() == ' ' || peek() == '\t'))
        advance();
}

// ─── Character classification ─────────────────────────────────────────────────

bool Tokenizer::isOperatorStart(char c) const
{
    return c == '|' || c == '&' || c == ';' || c == '<' || c == '>' ||
           c == '(' || c == ')' || c == '{' || c == '}';
}

bool Tokenizer::isWordChar(char c) const
{
    return c != '\0' && c != '\n' && c != ' ' && c != '\t' &&
           !isOperatorStart(c) && c != '\'' && c != '"';
}

// ─── Single-quoted string ─────────────────────────────────────────────────────

Token Tokenizer::readSingleQuoted()
{
    int tokLine = m_line;
    int tokCol  = m_col;
    advance(); // opening '

    std::string value;
    while (!atEnd() && peek() != '\'')
        value += advance();

    if (atEnd())
        throw std::runtime_error("unterminated single-quoted string at line " +
                                 std::to_string(tokLine));
    advance(); // closing '
    return { TokenType::SingleQuoted, value, -1, tokLine, tokCol };
}

// ─── Double-quoted string ─────────────────────────────────────────────────────

Token Tokenizer::readDoubleQuoted()
{
    int tokLine = m_line;
    int tokCol  = m_col;
    advance(); // opening "

    std::string value;
    while (!atEnd() && peek() != '"')
    {
        if (peek() == '\\' && m_pos + 1 < m_input.size())
        {
            value += advance(); // backslash
            value += advance(); // escaped char
        }
        else
        {
            value += advance();
        }
    }

    if (atEnd())
        throw std::runtime_error("unterminated double-quoted string at line " +
                                 std::to_string(tokLine));
    advance(); // closing "
    return { TokenType::DoubleQuoted, value, -1, tokLine, tokCol };
}

// ─── Word ─────────────────────────────────────────────────────────────────────

Token Tokenizer::readWord()
{
    int tokLine = m_line;
    int tokCol  = m_col;

    // ── Fd-prefixed redirect: single digit immediately followed by < or > ──
    if (std::isdigit(static_cast<unsigned char>(peek())) &&
        (peek(1) == '<' || peek(1) == '>'))
    {
        int  fd    = peek() - '0';
        advance(); // consume digit

        int  opLine = m_line;
        int  opCol  = m_col;
        char op     = advance(); // consume < or >

        if (op == '<')
        {
            if (peek() == '<')
            {
                advance();
                if (peek() == '-') { advance(); return { TokenType::HereDocDash,  "", fd, opLine, opCol }; }
                if (peek() == '<') { advance(); return { TokenType::HereString,   "", fd, opLine, opCol }; }
                return { TokenType::HereDoc, "", fd, opLine, opCol };
            }
            if (peek() == '&') { advance(); return { TokenType::DupIn,        "", fd, opLine, opCol }; }
            if (peek() == '>') { advance(); return { TokenType::RedirReadWrite,"", fd, opLine, opCol }; }
            return { TokenType::RedirIn, "", fd, opLine, opCol };
        }
        else // '>'
        {
            if (peek() == '>') { advance(); return { TokenType::RedirAppend,  "", fd, opLine, opCol }; }
            if (peek() == '&') { advance(); return { TokenType::DupOut,       "", fd, opLine, opCol }; }
            if (peek() == '|') { advance(); return { TokenType::RedirClobber, "", fd, opLine, opCol }; }
            return { TokenType::RedirOut, "", fd, opLine, opCol };
        }
    }

    // ── Regular word ──────────────────────────────────────────────────────
    std::string value;
    while (!atEnd() && isWordChar(peek()))
    {
        if (peek() == '\\' && m_pos + 1 < m_input.size())
        {
            char next = m_input[m_pos + 1];
            if (next == '\n')
            {
                advance(); // backslash
                advance(); // newline — line continuation, discard both
                skipWhitespace();
            }
            else
            {
                advance();          // backslash
                value += advance(); // escaped char taken literally
            }
        }
        else
        {
            value += advance();
        }
    }

    TokenType type = resolveKeyword(value);
    return { type, value, -1, tokLine, tokCol };
}

// ─── Keyword resolution ───────────────────────────────────────────────────────

TokenType Tokenizer::resolveKeyword(const std::string& word)
{
    static const std::unordered_map<std::string, TokenType> kw = {
        { "if",       TokenType::If       },
        { "then",     TokenType::Then     },
        { "elif",     TokenType::Elif     },
        { "else",     TokenType::Else     },
        { "fi",       TokenType::Fi       },
        { "while",    TokenType::While    },
        { "until",    TokenType::Until    },
        { "do",       TokenType::Do       },
        { "done",     TokenType::Done     },
        { "for",      TokenType::For      },
        { "in",       TokenType::In       },
        { "case",     TokenType::Case     },
        { "esac",     TokenType::Esac     },
        { "select",   TokenType::Select   },
        { "function", TokenType::Function },
        { "time",     TokenType::Time     },
    };

    auto it = kw.find(word);
    return (it != kw.end()) ? it->second : TokenType::Word;
}

// ─── Main tokenize loop ───────────────────────────────────────────────────────

std::vector<Token> Tokenizer::tokenize()
{
    std::vector<Token> tokens;

    while (!atEnd())
    {
        skipWhitespace();
        if (atEnd()) break;

        int  tokLine = m_line;
        int  tokCol  = m_col;
        char c       = peek();

        // ── Newline ───────────────────────────────────────────────────────
        if (c == '\n')
        {
            tokens.push_back({ TokenType::Newline, "", -1, tokLine, tokCol });
            advance();
            continue;
        }

        // ── Comment ───────────────────────────────────────────────────────
        if (c == '#')
        {
            while (!atEnd() && peek() != '\n')
                advance();
            continue;
        }

        // ── Operators — greedy longest-match ──────────────────────────────

        if (c == '|')
        {
            advance();
            if      (peek() == '|') { advance(); tokens.push_back({ TokenType::Or,       "||", -1, tokLine, tokCol }); }
            else if (peek() == '&') { advance(); tokens.push_back({ TokenType::PipeBoth, "|&", -1, tokLine, tokCol }); }
            else                    {            tokens.push_back({ TokenType::Pipe,      "|",  -1, tokLine, tokCol }); }
            continue;
        }

        if (c == '&')
        {
            advance();
            if (peek() == '&')
            {
                advance();
                tokens.push_back({ TokenType::And, "&&", -1, tokLine, tokCol });
            }
            else if (peek() == '>')
            {
                advance();
                if (peek() == '>') { advance(); tokens.push_back({ TokenType::RedirBothAppend, "&>>", -1, tokLine, tokCol }); }
                else               {            tokens.push_back({ TokenType::RedirBoth,        "&>",  -1, tokLine, tokCol }); }
            }
            else
            {
                tokens.push_back({ TokenType::Background, "&", -1, tokLine, tokCol });
            }
            continue;
        }

        if (c == ';')
        {
            advance();
            if (peek() == ';')
            {
                advance();
                if (peek() == '&') { advance(); tokens.push_back({ TokenType::DoubleSemiAmp, ";;&", -1, tokLine, tokCol }); }
                else               {            tokens.push_back({ TokenType::DoubleSemi,     ";;",  -1, tokLine, tokCol }); }
            }
            else if (peek() == '&') { advance(); tokens.push_back({ TokenType::SemiAmp, ";&", -1, tokLine, tokCol }); }
            else                    {            tokens.push_back({ TokenType::Semi,     ";",  -1, tokLine, tokCol }); }
            continue;
        }

        if (c == '<')
        {
            advance();
            if (peek() == '<')
            {
                advance();
                if      (peek() == '-') { advance(); tokens.push_back({ TokenType::HereDocDash,  "<<-", -1, tokLine, tokCol }); }
                else if (peek() == '<') { advance(); tokens.push_back({ TokenType::HereString,   "<<<", -1, tokLine, tokCol }); }
                else                    {            tokens.push_back({ TokenType::HereDoc,      "<<",  -1, tokLine, tokCol }); }
            }
            else if (peek() == '&') { advance(); tokens.push_back({ TokenType::DupIn,        "<&", -1, tokLine, tokCol }); }
            else if (peek() == '>') { advance(); tokens.push_back({ TokenType::RedirReadWrite,"<>", -1, tokLine, tokCol }); }
            else                    {            tokens.push_back({ TokenType::RedirIn,       "<",  -1, tokLine, tokCol }); }
            continue;
        }

        if (c == '>')
        {
            advance();
            if      (peek() == '>') { advance(); tokens.push_back({ TokenType::RedirAppend,  ">>", -1, tokLine, tokCol }); }
            else if (peek() == '&') { advance(); tokens.push_back({ TokenType::DupOut,       ">&", -1, tokLine, tokCol }); }
            else if (peek() == '|') { advance(); tokens.push_back({ TokenType::RedirClobber, ">|", -1, tokLine, tokCol }); }
            else                    {            tokens.push_back({ TokenType::RedirOut,      ">",  -1, tokLine, tokCol }); }
            continue;
        }

        if (c == '(')
        {
            advance();
            if (peek() == '(') { advance(); tokens.push_back({ TokenType::DLParen, "((", -1, tokLine, tokCol }); }
            else               {            tokens.push_back({ TokenType::LParen,  "(",  -1, tokLine, tokCol }); }
            continue;
        }

        if (c == ')')
        {
            advance();
            if (peek() == ')') { advance(); tokens.push_back({ TokenType::DRParen, "))", -1, tokLine, tokCol }); }
            else               {            tokens.push_back({ TokenType::RParen,  ")",  -1, tokLine, tokCol }); }
            continue;
        }

        if (c == '{') { advance(); tokens.push_back({ TokenType::LBrace, "{", -1, tokLine, tokCol }); continue; }
        if (c == '}') { advance(); tokens.push_back({ TokenType::RBrace, "}", -1, tokLine, tokCol }); continue; }

        // ── [[ and ]] ─────────────────────────────────────────────────────
        if (c == '[' && peek(1) == '[')
        {
            advance(); advance();
            tokens.push_back({ TokenType::DLBracket, "[[", -1, tokLine, tokCol });
            continue;
        }
        if (c == ']' && peek(1) == ']')
        {
            advance(); advance();
            tokens.push_back({ TokenType::DRBracket, "]]", -1, tokLine, tokCol });
            continue;
        }

        // ── ! ─────────────────────────────────────────────────────────────
        if (c == '!') { advance(); tokens.push_back({ TokenType::Bang, "!", -1, tokLine, tokCol }); continue; }

        // ── Quoted strings ────────────────────────────────────────────────
        if (c == '\'') { tokens.push_back(readSingleQuoted()); continue; }
        if (c == '"')  { tokens.push_back(readDoubleQuoted()); continue; }

        // ── Word (includes fd-prefixed redirects) ─────────────────────────
        tokens.push_back(readWord());
    }

    tokens.push_back({ TokenType::Eof, "", -1, m_line, m_col });
    return tokens;
}

} // namespace parser
