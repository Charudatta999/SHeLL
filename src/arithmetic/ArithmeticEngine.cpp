#include "arithmetic/ArithmeticEngine.hpp"

#include "arithmetic/ArithmeticException.hpp"
#include "arithmetic/ArithmeticVars.hpp"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
enum class Kind : int8_t
{
    Number,
    Ident,
    End, // cursor sentinel (Peek past end)

    // assignment
    Assign,    // =
    PlusEq,    // +=
    MinusEq,   // -=
    StarEq,    // *=
    SlashEq,   // /=
    PercentEq, // %=
    AmpEq,     // &=
    PipeEq,    // |=
    CaretEq,   // ^=
    ShlEq,     // <<=
    ShrEq,     // >>=
    ExpoEq,    // **=

    // ternary
    Question, // ?
    Colon,    // :

    // logical
    OrOr,   // ||
    AndAnd, // &&
    Not,    // !

    // bitwise
    Pipe,  // |
    Caret, // ^
    Amp,   // &
    Tilde, // ~
    Shl,   // <<
    Shr,   // >>

    // comparison
    EqEq, // ==
    Ne,   // !=
    Lt,   // <
    Le,   // <=
    Gt,   // >
    Ge,   // >=

    // arithmetic
    Plus,     // +
    Minus,    // -
    Negative, // unary -
    Star,     // *
    Slash,    // /
    Percent,  // %
    Expo,     // **

    // inc / dec
    Incr, // ++
    Decr, // --

    // grouping
    LParen, // (
    RParen, // )
};

std::string GetOptrString(Kind kind)
{
    switch (kind)
    {
        case Kind::Assign:
            return "=";
        case Kind::PlusEq:
            return "+=";
        case Kind::MinusEq:
            return "-=";
        case Kind::StarEq:
            return "*=";
        case Kind::SlashEq:
            return "/=";
        case Kind::PercentEq:
            return "%=";
        case Kind::AmpEq:
            return "&=";
        case Kind::PipeEq:
            return "|=";
        case Kind::CaretEq:
            return "^=";
        case Kind::ShlEq:
            return "<<=";
        case Kind::ShrEq:
            return ">>=";
        case Kind::ExpoEq:
            return "**=";

        case Kind::Question:
            return "?";
        case Kind::Colon:
            return ":";

        case Kind::OrOr:
            return "||";
        case Kind::AndAnd:
            return "&&";
        case Kind::Not:
            return "!";

        case Kind::Pipe:
            return "|";
        case Kind::Caret:
            return "^";
        case Kind::Amp:
            return "&";
        case Kind::Tilde:
            return "~";
        case Kind::Shl:
            return "<<";
        case Kind::Shr:
            return ">>";

        case Kind::EqEq:
            return "==";
        case Kind::Ne:
            return "!=";
        case Kind::Lt:
            return "<";
        case Kind::Le:
            return "<=";
        case Kind::Gt:
            return ">";
        case Kind::Ge:
            return ">=";

        case Kind::Plus:
            return "+";
        case Kind::Minus:
            return "-";
        case Kind::Negative:
            return "-";
        case Kind::Star:
            return "*";
        case Kind::Slash:
            return "/";
        case Kind::Percent:
            return "%";
        case Kind::Expo:
            return "**";

        case Kind::Incr:
            return "++";
        case Kind::Decr:
            return "--";

        case Kind::LParen:
            return "(";
        case Kind::RParen:
            return ")";

        default:
            return ""; // Number, Ident, End
    }
}
bool IsAssignOp(Kind kind)
{
    switch (kind)
    {
        case Kind::Assign: case Kind::PlusEq: case Kind::MinusEq:
        case Kind::StarEq: case Kind::SlashEq: case Kind::PercentEq:
        case Kind::AmpEq:  case Kind::PipeEq:  case Kind::CaretEq:
        case Kind::ShlEq:  case Kind::ShrEq:   case Kind::ExpoEq: return true;
        default: return false;
    }
}

struct Token
{
    Kind kind;
    std::int64_t number = 0;
    std::string ident;
};

std::int64_t ApplyBinary(std::int64_t lhs, std::int64_t rhs, Kind optr)
{
    switch (optr)
    {
        case Kind::Plus:    return lhs + rhs;
        case Kind::Minus:   return lhs - rhs;
        case Kind::Star:    return lhs * rhs;
        case Kind::Slash:
            if (rhs == 0) throw arithmetic::ArithmeticException("division by zero");
            return lhs / rhs;
        case Kind::Percent:
            if (rhs == 0) throw arithmetic::ArithmeticException("modulo by zero");
            return lhs % rhs;
        case Kind::Amp:     return lhs & rhs;
        case Kind::Pipe:    return lhs | rhs;
        case Kind::Caret:   return lhs ^ rhs;
        case Kind::Shl:     return lhs << rhs;
        case Kind::Shr:     return lhs >> rhs;
        case Kind::Lt:      return lhs <  rhs;
        case Kind::Le:      return lhs <= rhs;
        case Kind::Gt:      return lhs >  rhs;
        case Kind::Ge:      return lhs >= rhs;
        case Kind::EqEq:    return lhs == rhs;
        case Kind::Ne:      return lhs != rhs;
        case Kind::Expo:
        {
            if (rhs < 0) throw arithmetic::ArithmeticException("negative exponent");
            std::int64_t result = 1;
            for (std::int64_t count = 0; count < rhs; ++count) result *= lhs;
            return result;
        }
        default: throw arithmetic::ArithmeticException("bad operator");
    }
}


class Lexer final
{
public:
    explicit Lexer(const std::string& expr) : m_expr_(expr) {}

    ~Lexer() = default;
    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    Lexer(Lexer&&) = delete;
    Lexer& operator=(Lexer&&) = delete;

    std::vector<Token> Tokenize();

private:
    char Peek(size_t offset = 0) const;
    std::vector<Token> m_tokens_;
    char Advance();
    [[nodiscard]]
    bool AtEnd() const;
    [[nodiscard]]
    bool IsOperatorStart(char chr) const;
    [[nodiscard]]
    bool IsIdentStart(char chr) const;
    [[nodiscard]]
    bool IsIdentChar(char chr) const;

    void SkipWhitespace();
    Token ReadNumber();
    Token ReadIdent();
    Token ReadOperator();

    std::string m_expr_;
    std::size_t m_pos_ = 0;
    std::size_t m_line_ = 1;
    std::size_t m_col_ = 1;
};

char Lexer::Peek(size_t offset) const
{
    size_t idx = m_pos_ + offset;
    if (idx >= m_expr_.size())
        return '\0';
    return m_expr_[idx];
}

void Lexer::SkipWhitespace()
{
    while (!AtEnd() && (Peek() == ' ' || Peek() == '\t')) // not '\n'
        Advance();
}

bool Lexer::IsOperatorStart(char chr) const
{
    return chr == '+' || chr == '-' || chr == '*' || chr == '<' ||
           chr == '>' || chr == '(' || chr == ')' || chr == '/' ||
           chr == '%' || chr == '&' || chr == '|' || chr == '!' ||
           chr == '=' || chr == '~' || chr == '^' || chr == '?' ||
           chr == ':';
}

bool Lexer::IsIdentStart(char chr) const
{
    return (std::isalpha(static_cast<unsigned char>(chr)) ||
            chr == '_');
}

bool Lexer::IsIdentChar(char chr) const
{
    return (std::isalnum(static_cast<unsigned char>(chr)) ||
            chr == '_');
}

bool Lexer::AtEnd() const
{
    return m_pos_ >= m_expr_.size();
}

Token Lexer::ReadNumber()
{
    int64_t outVal{0};
    while (!AtEnd() &&
           std::isdigit(static_cast<unsigned char>(Peek())))
    {
        outVal = ((outVal * 10) + (Advance() - '0'));
    }
    return {.kind = Kind::Number, .number = outVal, .ident = ""};
}

char Lexer::Advance()
{
    char chr = m_expr_[m_pos_++];
    if (chr == '\n')
    {
        ++m_line_;
        m_col_ = 1;
    }
    else
    {
        ++m_col_;
    }
    return chr;
}

Token Lexer::ReadOperator()
{
    std::string optr;

    SkipWhitespace();
    if (!AtEnd() && !std::isdigit(static_cast<unsigned char>(Peek())))
    {
        optr += Advance();
        std::string nxtoptr = optr + Peek();
        if ((nxtoptr == "&&") || (nxtoptr == "||") ||
            (nxtoptr == "<=") || (nxtoptr == ">=") ||
            (nxtoptr == "==") || (nxtoptr == "!=") ||
            (nxtoptr == "+=") || (nxtoptr == "-=") ||
            (nxtoptr == "*=") || (nxtoptr == "/=") ||
            (nxtoptr == "%=") || (nxtoptr == "--") ||
            (nxtoptr == "++") || (nxtoptr == "**") ||
            (nxtoptr == "<<") || (nxtoptr == ">>") ||
            (nxtoptr == "&=") || (nxtoptr == "|=") ||
            (nxtoptr == "^="))
        {
            optr += Advance();
            // 3-char operators:  <<=  >>=  **=
            if ((optr == "<<" || optr == ">>" || optr == "**") &&
                Peek() == '=')
            {
                optr += Advance();
            }
        }

        if ("+" == optr)
        {
            return {.kind = Kind::Plus, .number = 0, .ident = ""};
        }
        else if ("-" == optr)
        {
            return {.kind = Kind::Minus, .number = 0, .ident = ""};
        }
        else if ("=" == optr)
        {
            return {.kind = Kind::Assign, .number = 0, .ident = ""};
        }
        else if ("*" == optr)
        {
            return {.kind = Kind::Star, .number = 0, .ident = ""};
        }
        else if ("<" == optr)
        {
            return {.kind = Kind::Lt, .number = 0, .ident = ""};
        }
        else if (">" == optr)
        {
            return {.kind = Kind::Gt, .number = 0, .ident = ""};
        }
        else if ("(" == optr)
        {
            return {.kind = Kind::LParen, .number = 0, .ident = ""};
        }
        else if (")" == optr)
        {
            return {.kind = Kind::RParen, .number = 0, .ident = ""};
        }
        else if ("/" == optr)
        {
            return {.kind = Kind::Slash, .number = 0, .ident = ""};
        }
        else if ("%" == optr)
        {
            return {.kind = Kind::Percent, .number = 0, .ident = ""};
        }
        else if ("&&" == optr)
        {
            return {.kind = Kind::AndAnd, .number = 0, .ident = ""};
        }
        else if ("||" == optr)
        {
            return {.kind = Kind::OrOr, .number = 0, .ident = ""};
        }
        else if ("!" == optr)
        {
            return {.kind = Kind::Not, .number = 0, .ident = ""};
        }
        else if ("==" == optr)
        {
            return {.kind = Kind::EqEq, .number = 0, .ident = ""};
        }
        else if ("!=" == optr)
        {
            return {.kind = Kind::Ne, .number = 0, .ident = ""};
        }
        else if ("<=" == optr)
        {
            return {.kind = Kind::Le, .number = 0, .ident = ""};
        }
        else if (">=" == optr)
        {
            return {.kind = Kind::Ge, .number = 0, .ident = ""};
        }
        else if ("+=" == optr)
        {
            return {.kind = Kind::PlusEq, .number = 0, .ident = ""};
        }
        else if ("-=" == optr)
        {
            return {.kind = Kind::MinusEq, .number = 0, .ident = ""};
        }
        else if ("*=" == optr)
        {
            return {.kind = Kind::StarEq, .number = 0, .ident = ""};
        }
        else if ("/=" == optr)
        {
            return {.kind = Kind::SlashEq, .number = 0, .ident = ""};
        }
        else if ("%=" == optr)
        {
            return {.kind = Kind::PercentEq,
                    .number = 0,
                    .ident = ""};
        }
        else if ("--" == optr)
        {
            return {.kind = Kind::Decr, .number = 0, .ident = ""};
        }
        else if ("++" == optr)
        {
            return {.kind = Kind::Incr, .number = 0, .ident = ""};
        }
        else if ("~" == optr)
        {
            return {.kind = Kind::Tilde, .number = 0, .ident = ""};
        }
        else if ("^" == optr)
        {
            return {.kind = Kind::Caret, .number = 0, .ident = ""};
        }
        else if ("&" == optr)
        {
            return {.kind = Kind::Amp, .number = 0, .ident = ""};
        }
        else if ("|" == optr)
        {
            return {.kind = Kind::Pipe, .number = 0, .ident = ""};
        }
        else if ("?" == optr)
        {
            return {.kind = Kind::Question, .number = 0, .ident = ""};
        }
        else if (":" == optr)
        {
            return {.kind = Kind::Colon, .number = 0, .ident = ""};
        }
        else if ("<<" == optr)
        {
            return {.kind = Kind::Shl, .number = 0, .ident = ""};
        }
        else if (">>" == optr)
        {
            return {.kind = Kind::Shr, .number = 0, .ident = ""};
        }
        else if ("**" == optr)
        {
            return {.kind = Kind::Expo, .number = 0, .ident = ""};
        }
        else if ("&=" == optr)
        {
            return {.kind = Kind::AmpEq, .number = 0, .ident = ""};
        }
        else if ("|=" == optr)
        {
            return {.kind = Kind::PipeEq, .number = 0, .ident = ""};
        }
        else if ("^=" == optr)
        {
            return {.kind = Kind::CaretEq, .number = 0, .ident = ""};
        }
        else if ("<<=" == optr)
        {
            return {.kind = Kind::ShlEq, .number = 0, .ident = ""};
        }
        else if (">>=" == optr)
        {
            return {.kind = Kind::ShrEq, .number = 0, .ident = ""};
        }
        else if ("**=" == optr)
        {
            return {.kind = Kind::ExpoEq, .number = 0, .ident = ""};
        }
        else
        {
            throw arithmetic::ArithmeticException("invalid Operator");
        }
    }
    else
    {
        throw arithmetic::ArithmeticException("invalid Operator");
    }
}

Token Lexer::ReadIdent()
{
    std::string identifier;
    while (!AtEnd() && IsIdentChar(Peek()))
    {
        identifier += Advance();
    }
    return {.kind = Kind::Ident, .number = 0, .ident = identifier};
}

std::vector<Token> Lexer::Tokenize()
{
    SkipWhitespace();
    while (!AtEnd())
    {
        char chr = Peek();
        if (std::isdigit(static_cast<unsigned char>(chr)))
            m_tokens_.push_back(ReadNumber());
        else if (IsIdentStart(chr))
            m_tokens_.push_back(ReadIdent());
        else if (IsOperatorStart(chr))
        {
            m_tokens_.push_back(ReadOperator());
        }
        else
            throw arithmetic::ArithmeticException(
                "unexpected character");
        SkipWhitespace();
    }
    m_tokens_.push_back({.kind = Kind::End, .number = 0, .ident = ""});
    return std::move(m_tokens_);
}

std::int64_t GetInt(const arithmetic::ArithmeticVars& vars,
                    const std::string& name)
{
    auto var = vars.Get(name);
    if (!var.has_value())
        return 0;
    try
    {
        return std::stoll(var.value());
    }
    catch (const std::exception&)
    {
        return 0; // non-numeric / empty / out-of-range -> 0
    }
}

class Parser final
{
public:
    Parser(std::vector<Token> tokens,
           arithmetic::ArithmeticVars& vars);
    std::int64_t
    Evaluate();

private:
    const Token& Peek(int offset = 0) const;
    const Token& Advance();
    bool Check(Kind kind) const;
    bool Match(Kind kind);
    const Token& Expect(Kind kind);
    bool AtEnd() const;
    Kind Base(Kind compound) const;

    std::int64_t ParseAssignment(bool eval);
    std::int64_t ParseTernary(bool eval);
    std::int64_t ParseLogicalAnd(bool eval);
    std::int64_t ParseLogicalOr(bool eval);
    std::int64_t ParseBitOr(bool eval);
    std::int64_t ParseBitXor(bool eval);
    std::int64_t ParseBitAnd(bool eval);
    std::int64_t ParseEquality(bool eval);
    std::int64_t ParseRelational(bool eval);
    std::int64_t ParseShift(bool eval);
    std::int64_t ParseAdditive(bool eval);
    std::int64_t ParseMultiplicative(bool eval);
    std::int64_t ParseUnary(bool eval);
    std::int64_t ParsePower(bool eval);
    std::int64_t ParsePrimary(bool eval);

    std::vector<Token> m_tokens_;
    std::size_t m_pos_ = 0;
    arithmetic::ArithmeticVars& m_vars_;
};

Kind Parser::Base(Kind compound) const
{
    switch (compound)
    {
        case Kind::PlusEq:    return Kind::Plus;
        case Kind::MinusEq:   return Kind::Minus;
        case Kind::StarEq:    return Kind::Star;
        case Kind::SlashEq:   return Kind::Slash;
        case Kind::PercentEq: return Kind::Percent;
        case Kind::AmpEq:     return Kind::Amp;
        case Kind::PipeEq:    return Kind::Pipe;
        case Kind::CaretEq:   return Kind::Caret;
        case Kind::ShlEq:     return Kind::Shl;
        case Kind::ShrEq:     return Kind::Shr;
        case Kind::ExpoEq:    return Kind::Expo;
        default:              return compound;
    }
}

const Token& Parser::Peek(int offset) const
{
    size_t idx = m_pos_ + static_cast<size_t>(offset);
    if (idx >= m_tokens_.size())
        return m_tokens_.back();
    return m_tokens_[idx];
}

const Token& Parser::Advance()
{
    const Token& tok = m_tokens_[m_pos_];
    if (m_pos_ + 1 < m_tokens_.size())
        ++m_pos_;
    return tok;
}

bool Parser::Check(Kind kind) const
{
    return Peek().kind == kind;
}

bool Parser::Match(Kind kind)
{
    if (!Check(kind))
        return false;
    Advance();
    return true;
}

Parser::Parser(std::vector<Token> tokens, arithmetic::ArithmeticVars& vars)
    : m_tokens_(std::move(tokens)), m_vars_(vars)
{
}

std::int64_t Parser::Evaluate()
{
    std::int64_t value = ParseAssignment(true);
    if (!AtEnd())
        throw arithmetic::ArithmeticException("unexpected trailing token");
    return value;
}

const Token& Parser::Expect(Kind kind)
{
    if (!Check(kind))
    {
        const Token& got = Peek();
        throw arithmetic::ArithmeticException(
            std::string("expected ") +
            std::string(GetOptrString(kind)) +
            "  got : " + std::string(GetOptrString(got.kind)));
    }
    return Advance();
}

bool Parser::AtEnd() const
{
    return Peek().kind == Kind::End;
}

std::int64_t Parser::ParseAssignment(bool eval)
{
    if (Peek().kind == Kind::Ident && IsAssignOp(Peek(1).kind))
    {
        std::string name = Advance().ident;
        Kind optr = Advance().kind;
        std::int64_t rhs = ParseAssignment(eval);
        if (!eval)
            return 0;
        std::int64_t val =
            (optr == Kind::Assign)
                ? rhs
                : ApplyBinary(GetInt(m_vars_, name), rhs,Base(optr));
        m_vars_.Set(name, std::to_string(val));
        return val;
    }
    return ParseTernary(eval);
}

std::int64_t Parser::ParseTernary(bool eval)
{
    std::int64_t cond = ParseLogicalOr(eval);
    if (Match(Kind::Question))
    {
        std::int64_t thenValue = ParseAssignment(eval && cond != 0);
        Expect(Kind::Colon);
        std::int64_t elseValue = ParseAssignment(eval && cond == 0);
        return eval ? (cond != 0 ? thenValue : elseValue) : 0;
    }
    return cond;
}

std::int64_t Parser::ParseLogicalOr(bool eval)
{
    std::int64_t left = ParseLogicalAnd(eval);
    while (Match(Kind::OrOr))
    {
        std::int64_t right = ParseLogicalAnd(eval && left == 0);
        if (eval)
            left = (left != 0 || right != 0) ? 1 : 0;
    }
    return left;
}

std::int64_t Parser::ParseLogicalAnd(bool eval)
{
    std::int64_t left = ParseBitOr(eval);
    while (Match(Kind::AndAnd))
    {
        std::int64_t right = ParseBitOr(eval && left != 0);
        if (eval)
            left = (left != 0 && right != 0) ? 1 : 0;
    }
    return left;
}

std::int64_t Parser::ParseBitOr(bool eval)
{
    std::int64_t left = ParseBitXor(eval);
    while (Match(Kind::Pipe))
    {
        std::int64_t right = ParseBitXor(eval);
        if (eval)
            left = ApplyBinary(left, right, Kind::Pipe);
    }
    return left;
}

std::int64_t Parser::ParseBitXor(bool eval)
{
    std::int64_t left = ParseBitAnd(eval);
    while (Match(Kind::Caret))
    {
        std::int64_t right = ParseBitAnd(eval);
        if (eval)
            left = ApplyBinary(left, right, Kind::Caret);
    }
    return left;
}

std::int64_t Parser::ParseBitAnd(bool eval)
{
    std::int64_t left = ParseEquality(eval);
    while (Match(Kind::Amp))
    {
        std::int64_t right = ParseEquality(eval);
        if (eval)
            left = ApplyBinary(left, right, Kind::Amp);
    }
    return left;
}

std::int64_t Parser::ParseEquality(bool eval)
{
    std::int64_t left = ParseRelational(eval);
    while (Check(Kind::EqEq) || Check(Kind::Ne))
    {
        Kind optr = Advance().kind;
        std::int64_t right = ParseRelational(eval);
        if (eval)
            left = ApplyBinary(left, right, optr);
    }
    return left;
}

std::int64_t Parser::ParseRelational(bool eval)
{
    std::int64_t left = ParseShift(eval);
    while (Check(Kind::Lt) || Check(Kind::Le) || Check(Kind::Gt) ||
           Check(Kind::Ge))
    {
        Kind optr = Advance().kind;
        std::int64_t right = ParseShift(eval);
        if (eval)
            left = ApplyBinary(left, right, optr);
    }
    return left;
}

std::int64_t Parser::ParseShift(bool eval)
{
    std::int64_t left = ParseAdditive(eval);
    while (Check(Kind::Shl) || Check(Kind::Shr))
    {
        Kind optr = Advance().kind;
        std::int64_t right = ParseAdditive(eval);
        if (eval)
            left = ApplyBinary(left, right, optr);
    }
    return left;
}

std::int64_t Parser::ParseAdditive(bool eval)
{
    std::int64_t left = ParseMultiplicative(eval);
    while (Check(Kind::Plus) || Check(Kind::Minus))
    {
        Kind optr = Advance().kind;
        std::int64_t right = ParseMultiplicative(eval);
        if (eval)
            left = ApplyBinary(left, right, optr);
    }
    return left;
}

std::int64_t Parser::ParseMultiplicative(bool eval)
{
    std::int64_t left = ParseUnary(eval);
    while (Check(Kind::Star) || Check(Kind::Slash) || Check(Kind::Percent))
    {
        Kind optr = Advance().kind;
        std::int64_t right = ParseUnary(eval);
        if (eval)
            left = ApplyBinary(left, right, optr);
    }
    return left;
}

std::int64_t Parser::ParseUnary(bool eval)
{
    if (Match(Kind::Minus))
    {
        std::int64_t value = ParseUnary(eval);
        return eval ? -value : 0;
    }
    if (Match(Kind::Plus))
    {
        return ParseUnary(eval);
    }
    if (Match(Kind::Not))
    {
        std::int64_t value = ParseUnary(eval);
        return eval ? (value == 0 ? 1 : 0) : 0;
    }
    if (Match(Kind::Tilde))
    {
        std::int64_t value = ParseUnary(eval);
        return eval ? ~value : 0;
    }
    if (Check(Kind::Incr) || Check(Kind::Decr))
    {
        Kind optr = Advance().kind;
        if (Peek().kind != Kind::Ident)
            throw arithmetic::ArithmeticException(
                "expected variable after ++/--");
        std::string name = Advance().ident;
        std::int64_t value = GetInt(m_vars_, name) + (optr == Kind::Incr ? 1 : -1);
        if (eval)
        {
            m_vars_.Set(name, std::to_string(value));
            return value;
        }
        return 0;
    }
    return ParsePower(eval);
}

std::int64_t Parser::ParsePower(bool eval)
{
    std::int64_t base = ParsePrimary(eval);
    if (Match(Kind::Expo))
    {
        std::int64_t exp = ParseUnary(eval);
        return eval ? ApplyBinary(base, exp, Kind::Expo) : 0;
    }
    return base;
}

std::int64_t Parser::ParsePrimary(bool eval)
{
    if (Match(Kind::LParen))
    {
        std::int64_t value = ParseAssignment(eval);
        Expect(Kind::RParen);
        return value;
    }
    if (Check(Kind::Number))
        return Advance().number;
    if (Check(Kind::Ident))
    {
        std::string name = Advance().ident;
        if (Check(Kind::Incr) || Check(Kind::Decr))
        {
            Kind optr = Advance().kind;
            std::int64_t old = GetInt(m_vars_, name);
            if (eval)
                m_vars_.Set(name,
                            std::to_string(old + (optr == Kind::Incr ? 1 : -1)));
            return eval ? old : 0;
        }
        return eval ? GetInt(m_vars_, name) : 0;
    }
    throw arithmetic::ArithmeticException("expected operand");
}


} // namespace

namespace arithmetic::engine
{

std::int64_t Evaluate(const std::string& expression,
                      ArithmeticVars& vars)
{
    Parser parser(Lexer(expression).Tokenize(), vars);
    return parser.Evaluate();
}
} // namespace arithmetic::engine