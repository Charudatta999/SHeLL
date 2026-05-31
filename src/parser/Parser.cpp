#include "parser/Parser.hpp"

#include "parser/Command.hpp"
#include "parser/ParserException.hpp"
#include "parser/Token.hpp"
#include "parser/ast/Ast.hpp"
#include "parser/ast/AstNode.hpp"
#include "parser/ast/Redirect.hpp"
#include "parser/ast/commands/AndOr.hpp"
#include "parser/ast/commands/Group.hpp"
#include "parser/ast/commands/SimpleCommand.hpp"
#include "parser/ast/commands/Subshell.hpp"
#include "parser/ast/commands/While.hpp"

#include <cctype>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
const char assignMentOp = '=';

std::pair<std::string, std::string> SplitOnce(const std::string& str,
                                              char delimiter)
{
    const auto pos = str.find(delimiter);
    if (pos == std::string_view::npos)
        return {str, ""};
    return {str.substr(0, pos), str.substr(pos + 1)};
}

} // namespace

namespace parser
{

// ─── Constructor
// ──────────────────────────────────────────────────────────────

Parser::Parser(std::vector<Token> tokens)
    : m_tokens_(std::move(tokens))
{
}

// ─── Token stream helpers
// ─────────────────────────────────────────────────────
bool Parser::IsAssignment(const std::string& str) const
{
    auto pos = str.find('=');
    if (pos == std::string::npos || pos == 0)
    {
        return false;
    }
    for (size_t i = 0; i < pos; ++i) // walk the KEY only: str[0..pos)
    {
        auto cha = static_cast<unsigned char>(str[i]);
        if (i == 0)
        {
            if (!std::isalpha(cha) && cha != '_')
                return false;
        }
        else
        {
            if (!std::isalnum(cha) && cha != '_')
                return false;
        }
    }
    return true;
}

const Token& Parser::Peek(int offset) const
{
    size_t idx = m_pos_ + static_cast<size_t>(offset);
    if (idx >= m_tokens_.size())
        return m_tokens_.back(); // Eof sentinel
    return m_tokens_[idx];
}

const Token& Parser::Advance()
{
    const Token& tok = m_tokens_[m_pos_];
    if (m_pos_ + 1 < m_tokens_.size())
        ++m_pos_;
    return tok;
}

bool Parser::Check(TokenType type) const
{
    return Peek().type == type;
}

bool Parser::Match(TokenType type)
{
    if (!Check(type))
        return false;
    Advance();
    return true;
}

const Token& Parser::Expect(TokenType type, const char* context)
{
    if (!Check(type))
    {
        const Token& got = Peek();
        throw ParserException(std::string("expected ") +
                                  std::string(tokenTypeName(type)) +
                                  " " + context + ", got '" +
                                  got.value + "'",
                              got.line,
                              got.col);
    }
    return Advance();
}

bool Parser::AtEnd() const
{
    return Peek().type == TokenType::Eof;
}

void Parser::SkipNewlines()
{
    while (Check(TokenType::Newline))
        Advance();
}

std::unique_ptr<ast::AstNode> Parser::ParseSimpleCommand()
{
    std::vector<std::string> argv;
    std::vector<ast::Redirect> redirects;
    std::vector<std::pair<std::string, std::string>> assignments;
    while (!AtEnd())
    {
        if (Check(TokenType::RedirAppend))
        {
            const auto& optr = Advance();
            if (!Check(TokenType::Word))
                throw ParserException("expected filename after >>",
                                      Peek().line,
                                      Peek().col);
            auto redir = ast::Redirect(ast::Redirect::Kind::Append,
                                       optr.fd,
                                       Advance().value);
            redirects.push_back(std::move(redir));
        }
        else if (Check(TokenType::RedirBothAppend))
        {
            const auto& optr = Advance();
            if (!Check(TokenType::Word))
                throw ParserException("expected filename after &>> ",
                                      Peek().line,
                                      Peek().col);
            redirects.emplace_back(ast::Redirect::Kind::BothAppend,
                                   optr.fd,
                                   Advance().value);
        }
        else if (Check(TokenType::RedirBoth))
        {
            const auto& optr = Advance();
            if (!Check(TokenType::Word))
                throw ParserException("expected filename after &>",
                                      Peek().line,
                                      Peek().col);
            redirects.emplace_back(ast::Redirect::Kind::Both,
                                   optr.fd,
                                   Advance().value);
        }
        else if (Check(TokenType::RedirIn))
        {
            const auto& optr = Advance();
            if (!Check(TokenType::Word))
                throw ParserException("expected filename after < ",
                                      Peek().line,
                                      Peek().col);
            redirects.emplace_back(ast::Redirect::Kind::In,
                                   optr.fd,
                                   Advance().value);
        }
        else if (Check(TokenType::RedirReadWrite))
        {
            const auto& optr = Advance();
            if (!Check(TokenType::Word))
                throw ParserException("expected filename after <> ",
                                      Peek().line,
                                      Peek().col);
            redirects.emplace_back(ast::Redirect::Kind::ReadWrite,
                                   optr.fd,
                                   Advance().value);
        }
        else if (Check(TokenType::RedirOut))
        {
            const auto& optr = Advance();
            if (!Check(TokenType::Word))
                throw ParserException("expected filename after >",
                                      Peek().line,
                                      Peek().col);
            redirects.emplace_back(ast::Redirect::Kind::Out,
                                   optr.fd,
                                   Advance().value);
        }
        else if (Check(TokenType::RedirClobber))
        {
            const auto& optr = Advance();
            if (!Check(TokenType::Word))
                throw ParserException("expected filename after >|",
                                      Peek().line,
                                      Peek().col);
            redirects.emplace_back(ast::Redirect::Kind::Clobber,
                                   optr.fd,
                                   Advance().value);
        }
        else if (Check(TokenType::Word))
        {
            std::string word = Advance().value;
            if (argv.empty() && IsAssignment(word))
                assignments.emplace_back(
                    SplitOnce(word, assignMentOp));
            else
                argv.emplace_back(std::move(word));
        }
        else
        {
            break;
        }
    }
    return std::make_unique<ast::SimpleCommand>(
        std::move(argv),
        std::move(redirects),
        std::move(assignments));
}

std::unique_ptr<ast::AstNode> Parser::ParsePipeline()
{
    auto first = ParseSimpleCommand();
    if (!Check(TokenType::Pipe))
    {
        return first;
    }
    std::vector<std::unique_ptr<ast::AstNode>> stages;
    stages.push_back(std::move(first));
    while (Match(TokenType::Pipe))
    {
        SkipNewlines();
        stages.push_back(ParseSimpleCommand());
    }
    return std::make_unique<ast::Pipeline>(std::move(stages), false);
}

std::unique_ptr<ast::AstNode> Parser::ParseAndOr()
{
    auto left = ParsePipeline();
    while (Check(TokenType::And) || Check(TokenType::Or))
    {
        ast::AndOr::Op optrtr = Check(TokenType::And)
                                    ? ast::AndOr::Op::And
                                    : ast::AndOr::Op::Or;
        Advance();
        SkipNewlines();
        auto right = ParsePipeline();
        left = std::make_unique<ast::AndOr>(std::move(left),
                                            optrtr,
                                            std::move(right));
    }
    return left;
}

std::unique_ptr<ast::AstNode> Parser::ParseList()
{
    std::vector<ast::List::Item> items;
    SkipNewlines();
    while (!AtEnd())
    {
        auto node = ParseAndOr();
        bool background = false;
        if (Match(TokenType::Background))
        {
            background = true;
        }
        else
        {
            if (Match(TokenType::Semi))
            {
            };
        }

        items.push_back(
            {.node = std::move(node), .background = background});
        SkipNewlines();
        if (AtEnd() ||
            (Check(TokenType::RBrace) || Check(TokenType::RParen)))
        {
            break;
        }
    }
    return std::make_unique<ast::List>(std::move(items));
}

std::unique_ptr<ast::AstNode> Parser::Parse()
{
    SkipNewlines();
    auto root = ParseList();
    if (!AtEnd())
    {
        const Token& bad = Peek();
        throw ParserException("unexpected token '" + bad.value + "'",
                              bad.line,
                              bad.col);
    }
    return root;
}

std::unique_ptr<ast::AstNode> Parser::ParseSubshell()
{
    Expect(TokenType::LParen, "at start of subshell");
    auto body = ParseList();
    Expect(TokenType::RParen, "to close subshell");
    return std::make_unique<ast::Subshell>(std::move(body));
}

std::unique_ptr<ast::AstNode> Parser::ParseGroup()
{
    Expect(TokenType::LBrace, "at start of group");
    auto body = ParseCommand();
    Expect(TokenType::RBrace, "to close group");
    return std::make_unique<ast::Group>(std::move(body));
}

std::unique_ptr<ast::AstNode> Parser::ParseFunction(std::string name)
{
    auto body = ParseCommand();
    return std::make_unique<ast::Function>(std::move(name),
                                           std::move(body));
}

std::unique_ptr<ast::AstNode> Parser::ParseWhile()
{
    bool until = Check(TokenType::Until);
    Advance();
    auto cond = ParseList();
    Expect(TokenType::Do, "Parsing (While|Until) loop");
    auto body = ParseList();
    Expect(TokenType::Done, "Parsing (While|Until) loop");
    return std::make_unique<ast::While>(std::move(cond),
                                        std::move(body),
                                        until);
}

std::unique_ptr<ast::AstNode> Parser::ParseFor()
{
    bool isFor = Check(TokenType::For);
    Advance();
    std::vector<std::string> words;
    std::unique_ptr<ast::AstNode> body;
    std::string var;
    if (isFor)
    {
        var =
            Expect(TokenType::Word, "Wrong Condiiton for loop").value;
        if (Match(TokenType::In))
        {
            while (Check(TokenType::Word))
            {
                words.push_back(Advance().value);
            }
        }
        if (Match(TokenType::Semi))
        {
        }
        SkipNewlines();
        Expect(TokenType::Do, "Invalid syntax for 'For loop'");
        body = ParseList();
        Expect(TokenType::Done, "Invalid syntax for 'For loop'");
    }
    else
    {
        var =
            Expect(TokenType::Word, "Wrong Condiiton for loop").value;
        Expect(TokenType::LParen,
               "Invalid syntax for 'For each loop'");
        while (Check(TokenType::Word))
        {
            words.push_back(Advance().value);
        }
        Expect(TokenType::RParen,
               "Invalid syntax for 'For each loop'");
        SkipNewlines();
        body = ParseList();
        Expect(TokenType::End, "Invalid syntax for 'For each loop'");
    }
    return std::make_unique<ast::For>(std::move(var),
                                      std::move(words),
                                      std::move(body));
}

std::unique_ptr<ast::AstNode> Parser::ParseIf()
{
    Expect(TokenType::If, "Invalid syntax for 'If / else'");
    auto cond = ParseList();
    Expect(TokenType::Then, "Invalid syntax for 'If / else'");
    auto body = ParseList();
    std::vector<ast::If::Branch> branches;
    std::unique_ptr<ast::AstNode> elseBody;
    branches.emplace_back(std::move(cond), std::move(body));
    while (Match(TokenType::Elif))
    {
        cond = ParseList();
        Expect(TokenType::Then, "Invalid syntax for 'If / else'");
        body = ParseList();
        branches.emplace_back(std::move(cond), std::move(body));
    }
    if (Match(TokenType::Else))
    {
        elseBody = ParseList();
    }
    Expect(TokenType::Fi, "Invalid syntax for 'If / else'");

    return std::make_unique<ast::If>(std::move(branches),
                                     std::move(elseBody));
}

std::unique_ptr<ast::AstNode> Parser::ParseCase()
{
    Expect(TokenType::Case, "Invalid syntax for 'Case'");
    auto word =
        Expect(TokenType::Word, "Invalid syntax for 'Case'").value;
    Expect(TokenType::In, "Invalid syntax for 'Case'");
    std::vector<ast::Case::Arm> arms;
    while (!Check(TokenType::Esac))
    {
        std::vector<std::string> patterns;
        if (Match(TokenType::LParen))
        {
        }
        patterns = {
            Expect(TokenType::Word, "Invalid syntax for 'Case'")
                .value};
        while (Match(TokenType::Pipe))
        {
            patterns.push_back(
                Expect(TokenType::Word, "Invalid syntax for 'Case'")
                    .value);
        }
        Expect(TokenType::RParen, "Invalid syntax for 'Case'");
        auto armBody = ParseList();
        if (Match(TokenType::DoubleSemi))
        {
        }
        arms.emplace_back(
            ast::Case::Arm(std::move(patterns), std::move(armBody)));
    }
    Expect(TokenType::Esac, "Invalid syntax for 'Case'");
    return std::make_unique<ast::Case>(std::move(word),
                                       std::move(arms));
}

std::unique_ptr<ast::AstNode> Parser::ParseCommand()
{
    SkipNewlines();
    const auto& typ = Peek().type;
    switch (typ)
    {
        case TokenType::LParen:
            return ParseSubshell();

        case TokenType::LBrace:
            return ParseGroup();

        case TokenType::If:
            return ParseIf();

        case TokenType::While:
        case TokenType::Until:
        {
            return ParseWhile();
        }
        case TokenType::For:
        case TokenType::Foreach:
            return ParseFor();
        case TokenType::Case:
            return ParseCase();
        case TokenType::Word:
        {
            if (Peek(1).type == TokenType::LParen &&
                Peek(2).type == TokenType::RParen)
            {
                const auto name = Advance().value;
                Advance(); // (
                Advance(); // )
                SkipNewlines();
                return ParseFunction(name);
            }
            return ParseSimpleCommand();
        }
        default:
            return ParseSimpleCommand();
    }
}
} // namespace parser
