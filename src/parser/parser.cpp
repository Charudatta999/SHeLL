#include "parser/Parser.hpp"

#include <cassert>

namespace parser
{

// ─── Constructor ──────────────────────────────────────────────────────────────

Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens))
{}

// ─── Token stream helpers ─────────────────────────────────────────────────────

const Token& Parser::peek(int offset) const
{
    size_t idx = m_pos + static_cast<size_t>(offset);
    if (idx >= m_tokens.size())
        return m_tokens.back(); // Eof sentinel
    return m_tokens[idx];
}

const Token& Parser::advance()
{
    const Token& tok = m_tokens[m_pos];
    if (m_pos + 1 < m_tokens.size()) ++m_pos;
    return tok;
}

bool Parser::check(TokenType type) const
{
    return peek().type == type;
}

bool Parser::match(TokenType type)
{
    if (!check(type)) return false;
    advance();
    return true;
}

const Token& Parser::expect(TokenType type, const char* context)
{
    if (!check(type))
    {
        const Token& got = peek();
        throw ParseError(
            std::string("expected ") + std::string(tokenTypeName(type)) +
            " " + context + ", got '" + got.value + "'",
            got.line, got.col);
    }
    return advance();
}

bool Parser::atEnd() const
{
    return peek().type == TokenType::Eof;
}

void Parser::skipNewlines()
{
    while (check(TokenType::Newline))
        advance();
}

// ─── Classification helpers ───────────────────────────────────────────────────

bool Parser::isWordLike(TokenType t) const
{
    switch (t)
    {
        case TokenType::Word:
        case TokenType::SingleQuoted:
        case TokenType::DoubleQuoted:
        // Keywords are word-like when used as arguments
        case TokenType::If:
        case TokenType::Then:
        case TokenType::Elif:
        case TokenType::Else:
        case TokenType::Fi:
        case TokenType::While:
        case TokenType::Until:
        case TokenType::Do:
        case TokenType::Done:
        case TokenType::For:
        case TokenType::In:
        case TokenType::Case:
        case TokenType::Esac:
        case TokenType::Select:
        case TokenType::Function:
        case TokenType::Time:
        case TokenType::Bang:
            return true;
        default:
            return false;
    }
}

bool Parser::isRedirectToken(TokenType t) const
{
    switch (t)
    {
        case TokenType::RedirIn:
        case TokenType::RedirOut:
        case TokenType::RedirAppend:
        case TokenType::RedirClobber:
        case TokenType::DupIn:
        case TokenType::DupOut:
        case TokenType::RedirBoth:
        case TokenType::RedirBothAppend:
        case TokenType::RedirReadWrite:
        case TokenType::HereDoc:
        case TokenType::HereDocDash:
        case TokenType::HereString:
            return true;
        default:
            return false;
    }
}

bool Parser::isPipelineTerminator(TokenType t) const
{
    switch (t)
    {
        case TokenType::Pipe:
        case TokenType::PipeBoth:
        case TokenType::And:
        case TokenType::Or:
        case TokenType::Semi:
        case TokenType::DoubleSemi:
        case TokenType::SemiAmp:
        case TokenType::DoubleSemiAmp:
        case TokenType::Background:
        case TokenType::Newline:
        case TokenType::RParen:
        case TokenType::RBrace:
        case TokenType::Eof:
        // Compound-command boundary keywords
        case TokenType::Then:
        case TokenType::Do:
        case TokenType::Done:
        case TokenType::Fi:
        case TokenType::Else:
        case TokenType::Elif:
        case TokenType::Esac:
        case TokenType::In:
            return true;
        default:
            return false;
    }
}

bool Parser::isListTerminator(TokenType t) const
{
    return t == TokenType::RParen ||
           t == TokenType::RBrace ||
           t == TokenType::Eof;
}

std::string Parser::tokenWord(const Token& tok) const
{
    return tok.value;
}

// ─── Redirect parsing ─────────────────────────────────────────────────────────

Redirect Parser::parseOneRedirect()
{
    const Token& op = advance(); // the redirect operator token
    Redirect r;

    switch (op.type)
    {
        case TokenType::RedirIn:       r.kind = Redirect::Kind::In;         break;
        case TokenType::RedirOut:      r.kind = Redirect::Kind::Out;        break;
        case TokenType::RedirAppend:   r.kind = Redirect::Kind::Append;     break;
        case TokenType::RedirClobber:  r.kind = Redirect::Kind::Clobber;    break;
        case TokenType::DupIn:         r.kind = Redirect::Kind::DupIn;      break;
        case TokenType::DupOut:        r.kind = Redirect::Kind::DupOut;     break;
        case TokenType::RedirBoth:     r.kind = Redirect::Kind::Both;       break;
        case TokenType::RedirBothAppend: r.kind = Redirect::Kind::BothAppend; break;
        case TokenType::RedirReadWrite:r.kind = Redirect::Kind::ReadWrite;  break;
        case TokenType::HereDoc:       r.kind = Redirect::Kind::HereDoc;    break;
        case TokenType::HereDocDash:   r.kind = Redirect::Kind::HereDocDash;break;
        case TokenType::HereString:    r.kind = Redirect::Kind::HereString; break;
        default:
            throw ParseError("internal: parseOneRedirect called on non-redirect",
                             op.line, op.col);
    }

    r.fd = op.fd; // -1 if not fd-prefixed

    // The target word must follow immediately
    if (!isWordLike(peek().type))
    {
        const Token& bad = peek();
        throw ParseError("expected filename/target after redirect operator",
                         bad.line, bad.col);
    }
    r.target = tokenWord(advance());
    return r;
}

void Parser::parseRedirects(std::vector<Redirect>& out)
{
    while (isRedirectToken(peek().type))
        out.push_back(parseOneRedirect());
}

// ─── parseList ────────────────────────────────────────────────────────────────
// list = andor (( ';' | '&' | '\n'+ ) andor)* (';' | '&')?

AstNodePtr Parser::parse()
{
    skipNewlines();
    AstNodePtr result = parseList();
    if (!atEnd() && !isListTerminator(peek().type))
    {
        const Token& bad = peek();
        throw ParseError("unexpected token '" + bad.value + "'", bad.line, bad.col);
    }
    return result;
}

AstNodePtr Parser::parseList()
{
    auto list = std::make_unique<ListNode>();
    list->line = peek().line;

    // Parse at least one item
    {
        AstNodePtr node = parseAndOr();
        bool bg = false;
        if (match(TokenType::Background))
            bg = true;
        else
            match(TokenType::Semi); // optional trailing ;
        skipNewlines();
        list->items.push_back({ std::move(node), bg });
    }

    // Continue while not at a list-terminating token
    while (!atEnd() && !isListTerminator(peek().type) &&
           !check(TokenType::Then) && !check(TokenType::Do) &&
           !check(TokenType::Done) && !check(TokenType::Fi) &&
           !check(TokenType::Else) && !check(TokenType::Elif) &&
           !check(TokenType::Esac))
    {
        AstNodePtr node = parseAndOr();
        bool bg = false;
        if (match(TokenType::Background))
            bg = true;
        else
            match(TokenType::Semi);
        skipNewlines();
        list->items.push_back({ std::move(node), bg });
    }

    // If only one item with no background flag, unwrap the list
    if (list->items.size() == 1 && !list->items[0].background)
        return std::move(list->items[0].node);

    return list;
}

// ─── parseAndOr ──────────────────────────────────────────────────────────────
// andor = pipeline (('&&' | '||') '\n'* pipeline)*

AstNodePtr Parser::parseAndOr()
{
    AstNodePtr lhs = parsePipeline();

    while (check(TokenType::And) || check(TokenType::Or))
    {
        AndOrNode::Op op = check(TokenType::And) ? AndOrNode::Op::And
                                                  : AndOrNode::Op::Or;
        advance();
        skipNewlines();

        auto node    = std::make_unique<AndOrNode>();
        node->line   = peek().line;
        node->lhs    = std::move(lhs);
        node->op     = op;
        node->rhs    = parsePipeline();
        lhs          = std::move(node);
    }

    return lhs;
}

// ─── parsePipeline ────────────────────────────────────────────────────────────
// pipeline = ['!'] command ('|' '\n'* command)*

AstNodePtr Parser::parsePipeline()
{
    bool bang = false;
    if (match(TokenType::Bang))
        bang = true;

    auto pipe = std::make_unique<PipelineNode>();
    pipe->line = peek().line;
    pipe->bang = bang;
    pipe->stages.push_back(parseCommand());

    while (check(TokenType::Pipe) || check(TokenType::PipeBoth))
    {
        advance();
        skipNewlines();
        pipe->stages.push_back(parseCommand());
    }

    // If single stage and no bang, unwrap to avoid unnecessary PipelineNode
    if (!bang && pipe->stages.size() == 1)
        return std::move(pipe->stages[0]);

    return pipe;
}

// ─── parseCommand ─────────────────────────────────────────────────────────────
// Dispatches to a compound command or simple command.

AstNodePtr Parser::parseCommand()
{
    skipNewlines();
    TokenType t = peek().type;

    if (t == TokenType::LParen)   return parseSubshell();
    if (t == TokenType::LBrace)   return parseGroup();
    if (t == TokenType::If)       return parseIf();
    if (t == TokenType::While)    return parseWhile();
    if (t == TokenType::Until)    return parseWhile(); // reuses parseWhile
    if (t == TokenType::For)      return parseFor();
    if (t == TokenType::Case)     return parseCase();
    if (t == TokenType::Function) { advance(); /* consume 'function' */ }

    // Function shorthand: word followed by ()
    if (t == TokenType::Word &&
        peek(1).type == TokenType::LParen &&
        peek(2).type == TokenType::RParen)
    {
        std::string name = tokenWord(advance()); // word
        advance(); // (
        advance(); // )
        skipNewlines();
        return parseFunction(name);
    }

    // 'function name' form (already consumed 'function' above if t == Function)
    if (t == TokenType::Function)
    {
        // peek() is now the function name
        if (!isWordLike(peek().type))
        {
            const Token& bad = peek();
            throw ParseError("expected function name", bad.line, bad.col);
        }
        std::string name = tokenWord(advance());
        // optional ()
        if (check(TokenType::LParen))
        {
            advance(); // (
            expect(TokenType::RParen, "after function name");
        }
        skipNewlines();
        return parseFunction(name);
    }

    return parseSimpleCommand();
}

// ─── parseSimpleCommand ───────────────────────────────────────────────────────

AstNodePtr Parser::parseSimpleCommand()
{
    auto cmd = std::make_unique<SimpleCommand>();
    cmd->line = peek().line;

    // Collect words, redirects, and leading VAR=value assignments
    bool pastAssignments = false;

    while (!isPipelineTerminator(peek().type))
    {
        // Redirect interspersed anywhere in the word list
        if (isRedirectToken(peek().type))
        {
            parseRedirects(cmd->redirects);
            continue;
        }

        if (!isWordLike(peek().type))
            break;

        const Token& tok = advance();
        std::string  w   = tokenWord(tok);

        // Leading VAR=value before the command word
        if (!pastAssignments)
        {
            // Check "IDENTIFIER=" prefix
            size_t eq = w.find('=');
            if (eq != std::string::npos && eq > 0)
            {
                std::string key = w.substr(0, eq);
                std::string val = w.substr(eq + 1);
                // Verify key is a valid identifier
                bool validId = true;
                for (size_t i = 0; i < key.size(); ++i)
                {
                    char c = key[i];
                    if (i == 0) validId = (std::isalpha(static_cast<unsigned char>(c)) || c == '_');
                    else        validId = (std::isalnum(static_cast<unsigned char>(c)) || c == '_');
                    if (!validId) break;
                }
                if (validId)
                {
                    cmd->assignments.emplace_back(std::move(key), std::move(val));
                    continue;
                }
            }
        }

        pastAssignments = true;
        cmd->argv.push_back(std::move(w));
    }

    // Trailing redirects after all words
    parseRedirects(cmd->redirects);

    if (cmd->argv.empty() && cmd->assignments.empty() && cmd->redirects.empty())
    {
        const Token& bad = peek();
        throw ParseError("empty command", bad.line, bad.col);
    }

    return cmd;
}

// ─── parseIf ─────────────────────────────────────────────────────────────────
// if cond; then body; (elif cond; then body;)* (else body;)? fi

AstNodePtr Parser::parseIf()
{
    int startLine = peek().line;
    expect(TokenType::If, "at start of if");

    auto node = std::make_unique<IfNode>();
    node->line = startLine;

    // First if branch
    {
        skipNewlines();
        AstNodePtr cond = parseList();
        skipNewlines();
        expect(TokenType::Then, "after if condition");
        skipNewlines();
        AstNodePtr body = parseList();
        skipNewlines();
        node->branches.push_back({ std::move(cond), std::move(body) });
    }

    // elif branches
    while (check(TokenType::Elif))
    {
        advance(); // consume elif
        skipNewlines();
        AstNodePtr cond = parseList();
        skipNewlines();
        expect(TokenType::Then, "after elif condition");
        skipNewlines();
        AstNodePtr body = parseList();
        skipNewlines();
        node->branches.push_back({ std::move(cond), std::move(body) });
    }

    // optional else
    if (match(TokenType::Else))
    {
        skipNewlines();
        node->else_body = parseList();
        skipNewlines();
    }

    expect(TokenType::Fi, "to close if statement");
    return node;
}

// ─── parseWhile ──────────────────────────────────────────────────────────────
// while cond; do body; done    (or until)

AstNodePtr Parser::parseWhile()
{
    auto node = std::make_unique<WhileNode>();
    node->line = peek().line;

    if (check(TokenType::Until))
    {
        node->until = true;
        advance();
    }
    else
    {
        expect(TokenType::While, "at start of while/until");
    }

    skipNewlines();
    node->condition = parseList();
    skipNewlines();
    expect(TokenType::Do, "after while/until condition");
    skipNewlines();
    node->body = parseList();
    skipNewlines();
    expect(TokenType::Done, "to close while/until");

    return node;
}

// ─── parseFor ────────────────────────────────────────────────────────────────
// for var [in word...]; do body; done

AstNodePtr Parser::parseFor()
{
    auto node = std::make_unique<ForNode>();
    node->line = peek().line;

    expect(TokenType::For, "at start of for");
    skipNewlines();

    if (!check(TokenType::Word))
    {
        const Token& bad = peek();
        throw ParseError("expected variable name after 'for'", bad.line, bad.col);
    }
    node->var = tokenWord(advance());
    skipNewlines();

    // Optional "in word..."
    if (match(TokenType::In))
    {
        while (isWordLike(peek().type))
            node->words.push_back(tokenWord(advance()));
        // consume trailing ; or newline
        if (!match(TokenType::Semi))
            skipNewlines();
    }
    else
    {
        // for var; do — iterates over "$@"
        match(TokenType::Semi);
    }

    skipNewlines();
    expect(TokenType::Do, "after for clause");
    skipNewlines();
    node->body = parseList();
    skipNewlines();
    expect(TokenType::Done, "to close for loop");

    return node;
}

// ─── parseCase ───────────────────────────────────────────────────────────────
// case word in (pattern|...) ) body ;; ... esac

AstNodePtr Parser::parseCase()
{
    auto node = std::make_unique<CaseNode>();
    node->line = peek().line;

    expect(TokenType::Case, "at start of case");
    skipNewlines();

    if (!isWordLike(peek().type))
    {
        const Token& bad = peek();
        throw ParseError("expected word after 'case'", bad.line, bad.col);
    }
    node->word = tokenWord(advance());
    skipNewlines();
    expect(TokenType::In, "after case word");
    skipNewlines();

    // Parse arms until esac
    while (!check(TokenType::Esac) && !atEnd())
    {
        CaseNode::Arm arm;

        // Optional leading (
        match(TokenType::LParen);

        // One or more patterns separated by |
        if (!isWordLike(peek().type))
        {
            const Token& bad = peek();
            throw ParseError("expected pattern in case arm", bad.line, bad.col);
        }
        arm.patterns.push_back(tokenWord(advance()));
        while (check(TokenType::Pipe))
        {
            advance();
            if (!isWordLike(peek().type))
            {
                const Token& bad = peek();
                throw ParseError("expected pattern after '|' in case", bad.line, bad.col);
            }
            arm.patterns.push_back(tokenWord(advance()));
        }

        expect(TokenType::RParen, "after case pattern");
        skipNewlines();

        // Arm body: parse until ;; ;& ;;&
        auto body = std::make_unique<ListNode>();
        body->line = peek().line;
        while (!check(TokenType::DoubleSemi) && !check(TokenType::SemiAmp) &&
               !check(TokenType::DoubleSemiAmp) && !check(TokenType::Esac) && !atEnd())
        {
            AstNodePtr item = parseAndOr();
            bool bg = false;
            if (match(TokenType::Background)) bg = true;
            else match(TokenType::Semi);
            skipNewlines();
            body->items.push_back({ std::move(item), bg });
        }
        if (body->items.size() == 1 && !body->items[0].background)
            arm.body = std::move(body->items[0].node);
        else if (!body->items.empty())
            arm.body = std::move(body);

        // Consume terminator
        if (check(TokenType::DoubleSemi) || check(TokenType::SemiAmp) ||
            check(TokenType::DoubleSemiAmp))
            advance();

        skipNewlines();
        node->arms.push_back(std::move(arm));
    }

    expect(TokenType::Esac, "to close case statement");
    return node;
}

// ─── parseSubshell ───────────────────────────────────────────────────────────
// ( list )

AstNodePtr Parser::parseSubshell()
{
    int startLine = peek().line;
    expect(TokenType::LParen, "at start of subshell");
    skipNewlines();

    auto node = std::make_unique<SubshellNode>();
    node->line = startLine;
    node->body = parseList();

    skipNewlines();
    expect(TokenType::RParen, "to close subshell");
    return node;
}

// ─── parseGroup ──────────────────────────────────────────────────────────────
// { list ; }

AstNodePtr Parser::parseGroup()
{
    int startLine = peek().line;
    expect(TokenType::LBrace, "at start of group command");
    skipNewlines();

    auto node = std::make_unique<GroupNode>();
    node->line = startLine;
    node->body = parseList();

    skipNewlines();
    // Require ; or newline before }
    if (!check(TokenType::RBrace))
    {
        const Token& bad = peek();
        throw ParseError("expected '}' to close group command", bad.line, bad.col);
    }
    advance();
    return node;
}

// ─── parseFunction ───────────────────────────────────────────────────────────
// name() compound-command

AstNodePtr Parser::parseFunction(const std::string& name)
{
    auto node  = std::make_unique<FunctionNode>();
    node->line = peek().line;
    node->name = name;
    node->body = parseCommand(); // must be a compound command
    return node;
}

} // namespace parser
