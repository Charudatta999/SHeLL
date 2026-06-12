#include "parser/Tokenizer.hpp"

#include "parser/ParserException.hpp"

#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>

namespace parser
{

// ─── Constructor
// ──────────────────────────────────────────────────────────────

Tokenizer::Tokenizer(const std::string& command) : m_command_(command)
{
}

char Tokenizer::Peek(int offset) const
{
    size_t idx = m_pos_ + static_cast<size_t>(offset);
    if (idx >= m_command_.size())
        return '\0';
    return m_command_[idx];
}

bool Tokenizer::AtEnd() const
{
    return m_pos_ >= m_command_.size();
}

void Tokenizer::SkipWhitespace()
{
    while (!AtEnd() && (Peek() == ' ' || Peek() == '\t')) // not '\n'
        Advance();
}

char Tokenizer::Advance()
{
    char c = m_command_[m_pos_++];
    if (c == '\n')
    {
        ++m_line_;
        m_col_ = 1;
    }
    else
    {
        ++m_col_;
    }
    return c;
}

bool Tokenizer::IsOperatorStart(char chr) const
{
    return chr == '|' || chr == '&' || chr == ';' || chr == '<' ||
           chr == '>' || chr == '(' || chr == ')' || chr == '{' ||
           chr == '}';
}

bool Tokenizer::IsWordChar(char chr) const
{
    return chr != '\0' && chr != '\n' && chr != ' ' && chr != '\t' &&
           !IsOperatorStart(chr) && chr != '\'' && chr != '"';
}

// ─── Single-quoted string
// ─────────────────────────────────────────────────────

Token Tokenizer::ReadSingleQuoted()
{
    size_t tokLine = m_line_;
    size_t tokCol = m_col_;
    Advance(); // opening '

    std::string value;
    while (!AtEnd() && Peek() != '\'')
        value += Advance();

    if (AtEnd())
        throw IncompleteInputException(
            "unterminated single-quoted string", tokLine, tokCol);
    Advance(); // closing '
    return Token{.type = TokenType::SingleQuoted,
                 .value = std::move(value),
                 .line = tokLine,
                 .col = tokCol};
}

// ─── Double-quoted string
// ─────────────────────────────────────────────────────

Token Tokenizer::ReadDoubleQuoted()
{
    size_t tokLine = m_line_;
    size_t tokCol = m_col_;
    Advance(); // opening "
    std::string value;
    while (!AtEnd() && Peek() != '"')
    {
        if (Peek() == '\\' && m_pos_ + 1 < m_command_.size())
        {
            value += Advance(); // backslash
            value += Advance(); // escaped char
        }
        else
        {
            value += Advance();
        }
    }

    if (AtEnd())
        throw IncompleteInputException(
            "unterminated double-quoted string", tokLine, tokCol);
    Advance(); // closing "
    return Token{.type = TokenType::DoubleQuoted,
                 .value = value,
                 .fd = -1,
                 .line = tokLine,
                 .col = tokCol};
}

// ─── Word
// ─────────────────────────────────────────────────────────────────────

Token Tokenizer::ReadWord()
{
    size_t tokLine = m_line_;
    size_t tokCol = m_col_;

    // ── Fd-prefixed redirect: single digit immediately followed by <
    // or > ──
    if (std::isdigit(static_cast<unsigned char>(Peek())) &&
        (Peek(1) == '<' || Peek(1) == '>'))
    {
        int fd = Peek() - '0';
        Advance(); // consume digit

        size_t opLine = m_line_;
        size_t opCol = m_col_;
        char op = Advance(); // consume < or >

        if (op == '<')
        {
            if (Peek() == '<')
            {
                Advance();
                if (Peek() == '-')
                {
                    Advance();
                    return Token{.type = TokenType::HereDocDash,
                                 .value = "",
                                 .fd = fd,
                                 .line = opLine,
                                 .col = opCol};
                }
                if (Peek() == '<')
                {
                    Advance();
                    return Token{.type = TokenType::HereString,
                                 .value = "",
                                 .fd = fd,
                                 .line = opLine,
                                 .col = opCol};
                }
                return Token{.type = TokenType::HereDoc,
                             .value = "",
                             .fd = fd,
                             .line = opLine,
                             .col = opCol};
            }
            if (Peek() == '&')
            {
                Advance();
                return {.type = TokenType::DupIn,
                        .value = "",
                        .fd = fd,
                        .line = opLine,
                        .col = opCol};
            }
            if (Peek() == '>')
            {
                Advance();
                return {.type = TokenType::RedirReadWrite,
                        .value = "",
                        .fd = fd,
                        .line = opLine,
                        .col = opCol};
            }
            return {.type = TokenType::RedirIn,
                    .value = "",
                    .fd = fd,
                    .line = opLine,
                    .col = opCol};
        }
        else // '>'
        {
            if (Peek() == '>')
            {
                Advance();
                return {.type = TokenType::RedirAppend,
                        .value = "",
                        .fd = fd,
                        .line = opLine,
                        .col = opCol};
            }
            if (Peek() == '&')
            {
                Advance();
                return {.type = TokenType::DupOut,
                        .value = "",
                        .fd = fd,
                        .line = opLine,
                        .col = opCol};
            }
            if (Peek() == '|')
            {
                Advance();
                return {.type = TokenType::RedirClobber,
                        .value = "",
                        .fd = fd,
                        .line = opLine,
                        .col = opCol};
            }
            return {.type = TokenType::RedirOut,
                    .value = "",
                    .fd = fd,
                    .line = opLine,
                    .col = opCol};
        }
    }

    // ── Regular word
    // ──────────────────────────────────────────────────────
    std::string value;
    while (!AtEnd() && IsWordChar(Peek()))
    {
        if (Peek() == '$')
        {

            if (Peek(1) == '(' && Peek(2) == '(')
            {
                value += Advance();
                value += Advance();
                value += Advance();
                value += ReadArithmeticBody() + "))";
                continue;
            }
            else if (Peek(1) == '(')
            {
                size_t depth = 1;
                value += Advance();
                value += Advance();
                while (!AtEnd())
                {
                    if (Peek() == '(')
                    {
                        ++depth;
                    }
                    else if (Peek() == ')')
                    {
                        --depth;
                    }
                    value += Advance();
                    if (depth == 0)
                    {
                        break;
                    }
                }
                if (depth != 0)
                {
                    throw IncompleteInputException(
                        "unterminated command substitution");
                }
            }
            else if (Peek(1) == '{')
            {
                value += Advance();
                value += Advance();
                if (Peek() == '}')
                {
                    continue;
                }
                while (!AtEnd() && Peek() != '}')
                {
                    value += Advance();
                }
                if (!AtEnd())
                {
                    value += Advance();
                }
                continue;
            }
        }
        if (Peek() == '\\' && m_pos_ + 1 < m_command_.size())
        {
            char next = m_command_[m_pos_ + 1];
            if (next == '\n')
            {
                Advance(); // backslash
                Advance(); // newline — line continuation, discard
                           // both
                SkipWhitespace();
            }
            else
            {
                Advance();          // backslash
                value += Advance(); // escaped char taken literally
            }
        }
        else
        {
            value += Advance();
        }
    }

    TokenType type = ResolveKeyword(value);
    return {.type = type,
            .value = value,
            .fd = -1,
            .line = tokLine,
            .col = tokCol};
}

// ─── Keyword resolution
// ───────────────────────────────────────────────────────

TokenType Tokenizer::ResolveKeyword(const std::string& word)
{
    static const std::unordered_map<std::string, TokenType> kw = {
        {"if", TokenType::If},
        {"then", TokenType::Then},
        {"elif", TokenType::Elif},
        {"else", TokenType::Else},
        {"fi", TokenType::Fi},
        {"while", TokenType::While},
        {"until", TokenType::Until},
        {"do", TokenType::Do},
        {"done", TokenType::Done},
        {"for", TokenType::For},
        {"foreach", TokenType::Foreach},
        {"end", TokenType::End},
        {"in", TokenType::In},
        {"case", TokenType::Case},
        {"esac", TokenType::Esac},
        {"select", TokenType::Select},
        {"function", TokenType::Function},
        {"time", TokenType::Time},
    };

    auto it = kw.find(word);
    return (it != kw.end()) ? it->second : TokenType::Word;
}

const std::string Tokenizer::ReadArithmeticBody()
{
    size_t tokLine = m_line_;

    std::string value;
    size_t depth{0};
    while (!AtEnd())
    {
        auto firstChar = Peek();
        if (firstChar == '(')
        {
            value += Advance();
            ++depth;
        }
        else if (firstChar == ')' && depth > 0)
        {
            value += Advance();
            --depth;
        }
        else if (firstChar == ')')
        {
            Advance();
            if (Peek() == ')')
            {
                Advance();
                return value;
            }
            throw std::runtime_error("malformed ((..))");
        }
        else
        {
            value += Advance();
        }
    }

    if (AtEnd())
        throw IncompleteInputException("unterminated (( expression",
                                       tokLine,
                                       0);
    return value;
}

// ─── Main tokenize loop
// ───────────────────────────────────────────────────────
std::vector<Token> Tokenizer::Tokenize()
{
    std::vector<Token> tokens;

    while (!AtEnd())
    {
        SkipWhitespace();
        if (AtEnd())
            break;

        size_t tokLine = m_line_;
        size_t tokCol = m_col_;
        char chr = Peek();

        // ── Newline
        // ───────────────────────────────────────────────────────
        if (chr == '\n')
        {
            tokens.push_back({.type = TokenType::Newline,
                              .value = "",
                              .fd = -1,
                              .line = tokLine,
                              .col = tokCol});
            Advance();
            continue;
        }

        // ── Comment
        // ───────────────────────────────────────────────────────
        if (chr == '#')
        {
            while (!AtEnd() && Peek() != '\n')
                Advance();
            continue;
        }

        // ── Operators — greedy longest-match
        // ──────────────────────────────

        if (chr == '|')
        {
            Advance();
            if (Peek() == '|')
            {
                Advance();
                tokens.push_back({.type = TokenType::Or,
                                  .value = "||",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else if (Peek() == '&')
            {
                Advance();
                tokens.push_back({.type = TokenType::PipeBoth,
                                  .value = "|&",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else
            {
                tokens.push_back({.type = TokenType::Pipe,
                                  .value = "|",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            continue;
        }

        if (chr == '&')
        {
            Advance();
            if (Peek() == '&')
            {
                Advance();
                tokens.push_back({.type = TokenType::And,
                                  .value = "&&",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else if (Peek() == '>')
            {
                Advance();
                if (Peek() == '>')
                {
                    Advance();
                    tokens.push_back(
                        {.type = TokenType::RedirBothAppend,
                         .value = "&>>",
                         .fd = -1,
                         .line = tokLine,
                         .col = tokCol});
                }
                else
                {
                    tokens.push_back({.type = TokenType::RedirBoth,
                                      .value = "&>",
                                      .fd = -1,
                                      .line = tokLine,
                                      .col = tokCol});
                }
            }
            else
            {
                tokens.push_back({.type = TokenType::Background,
                                  .value = "&",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            continue;
        }

        if (chr == ';')
        {
            Advance();
            if (Peek() == ';')
            {
                Advance();
                if (Peek() == '&')
                {
                    Advance();
                    tokens.push_back(
                        {.type = TokenType::DoubleSemiAmp,
                         .value = ";;&",
                         .fd = -1,
                         .line = tokLine,
                         .col = tokCol});
                }
                else
                {
                    tokens.push_back({.type = TokenType::DoubleSemi,
                                      .value = ";;",
                                      .fd = -1,
                                      .line = tokLine,
                                      .col = tokCol});
                }
            }
            else if (Peek() == '&')
            {
                Advance();
                tokens.push_back({.type = TokenType::SemiAmp,
                                  .value = ";&",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else
            {
                tokens.push_back({.type = TokenType::Semi,
                                  .value = ";",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            continue;
        }

        if (chr == '<')
        {
            Advance();
            if (Peek() == '<')
            {
                Advance();
                if (Peek() == '-')
                {
                    Advance();
                    tokens.push_back({.type = TokenType::HereDocDash,
                                      .value = "<<-",
                                      .fd = -1,
                                      .line = tokLine,
                                      .col = tokCol});
                }
                else if (Peek() == '<')
                {
                    Advance();
                    tokens.push_back({.type = TokenType::HereString,
                                      .value = "<<<",
                                      .fd = -1,
                                      .line = tokLine,
                                      .col = tokCol});
                }
                else
                {
                    tokens.push_back({.type = TokenType::HereDoc,
                                      .value = "<<",
                                      .fd = -1,
                                      .line = tokLine,
                                      .col = tokCol});
                }
            }
            else if (Peek() == '&')
            {
                Advance();
                tokens.push_back({.type = TokenType::DupIn,
                                  .value = "<&",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else if (Peek() == '>')
            {
                Advance();
                tokens.push_back({.type = TokenType::RedirReadWrite,
                                  .value = "<>",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else
            {
                tokens.push_back({.type = TokenType::RedirIn,
                                  .value = "<",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            continue;
        }

        if (chr == '>')
        {
            Advance();
            if (Peek() == '>')
            {
                Advance();
                tokens.push_back({.type = TokenType::RedirAppend,
                                  .value = ">>",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else if (Peek() == '&')
            {
                Advance();
                tokens.push_back({.type = TokenType::DupOut,
                                  .value = ">&",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else if (Peek() == '|')
            {
                Advance();
                tokens.push_back({.type = TokenType::RedirClobber,
                                  .value = ">|",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else
            {
                tokens.push_back({.type = TokenType::RedirOut,
                                  .value = ">",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            continue;
        }

        if (chr == '(')
        {
            Advance();
            if (Peek() == '(')
            {
                Advance();
                auto line = ReadArithmeticBody();
                tokens.push_back({.type = TokenType::DLParen,
                                  .value = line,
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else
            {
                tokens.push_back({.type = TokenType::LParen,
                                  .value = "(",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            continue;
        }

        if (chr == ')')
        {
            Advance();
            if (Peek() == ')')
            {
                Advance();
                tokens.push_back({.type = TokenType::DRParen,
                                  .value = "))",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            else
            {
                tokens.push_back({.type = TokenType::RParen,
                                  .value = ")",
                                  .fd = -1,
                                  .line = tokLine,
                                  .col = tokCol});
            }
            continue;
        }

        if (chr == '{')
        {
            Advance();
            tokens.push_back({.type = TokenType::LBrace,
                              .value = "{",
                              .fd = -1,
                              .line = tokLine,
                              .col = tokCol});
            continue;
        }
        if (chr == '}')
        {
            Advance();
            tokens.push_back({.type = TokenType::RBrace,
                              .value = "}",
                              .fd = -1,
                              .line = tokLine,
                              .col = tokCol});
            continue;
        }

        // ── [[ and ]]
        // ─────────────────────────────────────────────────────
        if (chr == '[' && Peek(1) == '[')
        {
            Advance();
            Advance();
            tokens.push_back({.type = TokenType::DLBracket,
                              .value = "[[",
                              .fd = -1,
                              .line = tokLine,
                              .col = tokCol});
            continue;
        }
        if (chr == ']' && Peek(1) == ']')
        {
            Advance();
            Advance();
            tokens.push_back({.type = TokenType::DRBracket,
                              .value = "]]",
                              .fd = -1,
                              .line = tokLine,
                              .col = tokCol});
            continue;
        }

        // ── !
        // ─────────────────────────────────────────────────────────────
        if (chr == '!')
        {
            Advance();
            tokens.push_back({.type = TokenType::Bang,
                              .value = "!",
                              .fd = -1,
                              .line = tokLine,
                              .col = tokCol});
            continue;
        }

        // ── Quoted strings
        // ────────────────────────────────────────────────
        if (chr == '\'')
        {
            tokens.push_back(ReadSingleQuoted());
            continue;
        }
        if (chr == '"')
        {
            tokens.push_back(ReadDoubleQuoted());
            continue;
        }

        // ── Word (includes fd-prefixed redirects)
        // ─────────────────────────
        tokens.push_back(ReadWord());
    }

    tokens.push_back({.type = TokenType::Eof,
                      .value = "",
                      .fd = -1,
                      .line = m_line_,
                      .col = m_col_});
    return tokens;
}

} // namespace parser
