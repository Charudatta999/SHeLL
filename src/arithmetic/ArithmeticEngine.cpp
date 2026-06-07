#include "arithmetic/ArithmeticEngine.hpp"

#include "arithmetic/ArithmeticVars.hpp"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
enum class Kind
{
    Number,
    Ident,
    Assign,
    Plus,
    Minus,
    Negative,
    Star,
    Slash,
    Percent,
    LParen,
    RParen,
    Lt,
    Gt,
    Le,
    Ge,
    EqEq,
    Ne,
    AndAnd,
    OrOr,
    Not,
    End,
    PlusEq,
    MinusEq,
    StarEq,
    SlashEq,
    PercentEq,
    Incr,
    Decr
};

struct Token
{
    Kind kind;
    std::int64_t number = 0;
    std::string ident;
};

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
    bool IsWordChar(char chr) const;

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
           chr == '=';
}

bool Lexer::IsWordChar(char chr) const
{
    return chr != '\0' && chr != '\n' && chr != ' ' && chr != '\t' &&
           !IsOperatorStart(chr) && chr != '\'' && chr != '"';
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
            (nxtoptr == "++"))
        {
            optr += Advance();
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
        else
        {
            throw std::runtime_error("invalid Operator");
        }
    }
    else
    {
        throw std::runtime_error("invalid Operator");
    }
}

Token Lexer::ReadIdent()
{
    std::string identifier;
    while (!AtEnd() && IsWordChar(Peek()))
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
        else if (IsWordChar(chr))
            m_tokens_.push_back(ReadIdent());
        else if (IsOperatorStart(chr))
        {
            auto optr = ReadOperator();
            bool prevIsOperand =
                (!m_tokens_.empty() &&
                 (m_tokens_.back().kind == Kind::Number ||
                  m_tokens_.back().kind == Kind::Ident ||
                  m_tokens_.back().kind == Kind::RParen));
            if (optr.kind == Kind::Minus && !prevIsOperand)
            {
                m_tokens_.push_back({.kind = Kind::Negative,
                                     .number = 0,
                                     .ident = ""});
            }
            else
            {
                m_tokens_.push_back(optr);
            }
        }
        else
            throw std::runtime_error("unexpected character");
        SkipWhitespace();
    }
    return std::move(m_tokens_);
}

int Prec(Kind kind)
{
    switch (kind)
    {
        case ::Kind::Negative:
        {
            return 7;
        }
        case Kind::Star:
        case Kind::Slash:
        case Kind::Percent:
        {
            return 6;
        }
        case Kind::Plus:
        case Kind::Minus:
        {
            return 5;
        }
        case Kind::Le:
        case Kind::Ge:
        case Kind::Gt:
        case Kind::Lt:
        {
            return 4;
        }
        case Kind::EqEq:
        case Kind::Ne:
        {
            return 3;
        }
        case Kind::AndAnd:
        {
            return 2;
        }
        case Kind::OrOr:
        {
            return 1;
        }

        default:
        {
            return 0;
        }
    }
}

int64_t Apply(int64_t firstNum, int64_t secondNum, Kind optr)
{
    switch (optr)
    {
        case Kind::Star:
        {
            return firstNum * secondNum;
        }
        case Kind::Slash:
        {
            if (secondNum == 0)
            {
                throw std::runtime_error("divide by zero error");
            }
            return firstNum / secondNum;
        }
        case Kind::Percent:
        {
            if (secondNum == 0)
            {
                throw std::runtime_error("divide by zero error");
            }
            return firstNum % secondNum;
        }
        case Kind::Plus:
        {
            return firstNum + secondNum;
        }
        case Kind::Minus:
        {
            return firstNum - secondNum;
        }
        case Kind::Le:
        {
            return firstNum <= secondNum;
        }
        case Kind::Ge:
        {
            return firstNum >= secondNum;
        }
        case Kind::Gt:
        {
            return firstNum > secondNum;
        }
        case Kind::Lt:
        {
            return firstNum < secondNum;
        }
        case Kind::EqEq:
        {
            return firstNum == secondNum;
        }
        case Kind::Ne:
        {
            return firstNum != secondNum;
        }
        case Kind::AndAnd:
        {
            return firstNum && secondNum;
        }
        case Kind::OrOr:
        {
            return firstNum || secondNum;
        }
        default:
            throw std::runtime_error("invalid args");
    }
}

void ApplyTop(std::vector<int64_t>& values, std::vector<Kind>& ops)
{
    auto optr = ops.back();
    ops.pop_back();
    if (optr == Kind::Negative)
    {
        auto val = values.back();
        values.pop_back();
        values.push_back(-val);
        return;
    }
    auto secondNum = values.back();
    values.pop_back();
    auto firstNum = values.back();
    values.pop_back();
    values.push_back(Apply(firstNum, secondNum, optr));
}

std::int64_t GetInt(const arithmetic::ArithmeticVars& vars,
                    const std::string& name)
{
    auto var = vars.Get(name);
    return var.has_value() ? std::stoll(var.value()) : 0;
}

int64_t Eval(const std::vector<Token>& tokens,
             arithmetic::ArithmeticVars& vars)
{
    std::vector<int64_t> values;
    std::vector<Kind> ops;
    for (const auto& token : tokens)
    {
        std::int64_t value{};
        if (token.kind == Kind::Number)
        {
            value = token.number;
            values.push_back(value);
        }
        else if (token.kind == Kind::Ident)
        {
            values.push_back(GetInt(vars, token.ident));
        }
        else if (token.kind == Kind::LParen)
        {
            ops.push_back(token.kind);
        }
        else if (token.kind == Kind::RParen)
        {
            while (ops.back() != Kind::LParen)
            {
                ApplyTop(values, ops);
            }
            ops.pop_back();
        }
        else if (token.kind == Kind::Negative)
        {
            ops.push_back(token.kind);
        }
        else
        {
            while (!ops.empty() && ops.back() != Kind::LParen &&
                   Prec(ops.back()) >= Prec(token.kind))
            {
                ApplyTop(values, ops);
            }
            ops.push_back(token.kind);
        }
    }
    while (!ops.empty())
    {
        ApplyTop(values, ops);
    }
    return values.back();
}
} // namespace

namespace arithmetic::engine
{

std::int64_t Evaluate(const std::string& expression,
                      ArithmeticVars& vars)
{
    auto tokens = Lexer(expression).Tokenize();

    if (tokens.size() < 2) {return Eval(tokens, vars);}

    auto& token1 = tokens[0];
    auto& token2 = tokens[1];

    if(token1.kind == Kind::Ident)
    {
        if (token2.kind == Kind::Incr)
        {
            auto oldVal = GetInt(vars, token1.ident);
            vars.Set(token1.ident,std::to_string(( oldVal + 1)));
            return oldVal;
        }
        else if (token2.kind == Kind::Decr)
        {
            auto oldVal = GetInt(vars, token1.ident);
            vars.Set(token1.ident,std::to_string((oldVal - 1)));
            return oldVal;
        }
        else if (token2.kind == Kind::Assign)
        {
            std::vector<Token> rest(tokens.begin() + 2, tokens.end());
            auto rhsResult = Eval(rest, vars);
            vars.Set(token1.ident, std::to_string(rhsResult)); return rhsResult;
        }
        else if (token2.kind == Kind::PlusEq)
        {
            auto oldVal = GetInt(vars, token1.ident);
             std::vector<Token> rest(tokens.begin() + 2, tokens.end());
            auto rhsResult = Eval(rest, vars);
            vars.Set(token1.ident,std::to_string((oldVal + rhsResult)));
            return oldVal + rhsResult;
        }
        else if (token2.kind == Kind::MinusEq) {
            auto oldVal = GetInt(vars, token1.ident);
             std::vector<Token> rest(tokens.begin() + 2, tokens.end());
            auto rhsResult = Eval(rest, vars);
            vars.Set(token1.ident,std::to_string((oldVal - rhsResult)));
            return oldVal - rhsResult;
        }
        else if (token2.kind == Kind::StarEq) {
            auto oldVal = GetInt(vars, token1.ident);
             std::vector<Token> rest(tokens.begin() + 2, tokens.end());
            auto rhsResult = Eval(rest, vars);
            vars.Set(token1.ident,std::to_string((oldVal * rhsResult)));
            return oldVal * rhsResult;
        }
        else if (token2.kind == Kind::SlashEq) {
            auto oldVal = GetInt(vars, token1.ident);
             std::vector<Token> rest(tokens.begin() + 2, tokens.end());
            auto rhsResult = Eval(rest, vars);
            vars.Set(token1.ident,std::to_string((Apply(oldVal, rhsResult, Kind::Slash))));
            return Apply(oldVal, rhsResult, Kind::Slash);
        }
        else if (token2.kind == Kind::PercentEq) {
            auto oldVal = GetInt(vars, token1.ident);
             std::vector<Token> rest(tokens.begin() + 2, tokens.end());
            auto rhsResult = Eval(rest, vars);
            vars.Set(token1.ident,std::to_string((Apply(oldVal, rhsResult, Kind::Percent))));
            return Apply(oldVal, rhsResult, Kind::Percent);
        }
    }
    else if (token1.kind != Kind::Number && token1.kind != Kind::Ident)
    {
        if(token1.kind == Kind::Incr)
        {
            auto  val = GetInt(vars, token2.ident);
            vars.Set(token2.ident,std::to_string(( val + 1)));
            return val +1;
        }
        else if(token1.kind == Kind::Decr)
        {
            auto  val = GetInt(vars, token2.ident);
            vars.Set(token2.ident,std::to_string(( val - 1)));
            return val - 1;
        }
    }

    return Eval(tokens, vars);
}
} // namespace arithmetic::engine