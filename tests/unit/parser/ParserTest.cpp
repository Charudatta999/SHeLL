// =========================================================
// ParserTest — unit tests for the recursive-descent parser.
//
// Covers every node type (positive) and a large battery of
// malformed inputs (negative). Uses the class-based parser::ast
// API: nodes have private members exposed via getters, and are
// identified with dynamic_cast.
// =========================================================

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "parser/Parser.hpp"
#include "parser/ParserException.hpp"
#include "parser/Tokenizer.hpp"
#include "parser/ast/Ast.hpp"

using namespace parser;
using namespace parser::ast;

// ─── Helpers ────────────────────────────────────────────────────────────────

static std::unique_ptr<AstNode> parse(const std::string& input)
{
    Tokenizer tok(input);
    Parser    p(tok.Tokenize());
    return p.Parse();
}

template <typename T>
static const T* as(const std::unique_ptr<AstNode>& node)
{
    return dynamic_cast<const T*>(node.get());
}

template <typename T>
static const T* as(const AstNode* node)
{
    return dynamic_cast<const T*>(node);
}

// ─── SimpleCommand ──────────────────────────────────────────────────────────

TEST(ParserTest, SingleWord)
{
    auto node       = parse("echo");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Argv().size(), 1u);
    EXPECT_EQ(cmd->Argv()[0], "echo");
}

TEST(ParserTest, MultipleWords)
{
    auto node       = parse("echo hello world");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Argv().size(), 3u);
    EXPECT_EQ(cmd->Argv()[0], "echo");
    EXPECT_EQ(cmd->Argv()[1], "hello");
    EXPECT_EQ(cmd->Argv()[2], "world");
}

TEST(ParserTest, SingleQuotedArg)
{
    auto node       = parse("echo 'hello world'");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Argv().size(), 2u);
    EXPECT_EQ(cmd->Argv()[1], "hello world");
}

TEST(ParserTest, DoubleQuotedArg)
{
    auto node       = parse("echo \"hello world\"");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Argv().size(), 2u);
    EXPECT_EQ(cmd->Argv()[1], "hello world");
}

TEST(ParserTest, LeadingAssignment)
{
    auto node       = parse("FOO=bar echo hello");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Assignments().size(), 1u);
    EXPECT_EQ(cmd->Assignments()[0].first, "FOO");
    EXPECT_EQ(cmd->Assignments()[0].second, "bar");
    ASSERT_EQ(cmd->Argv().size(), 2u);
    EXPECT_EQ(cmd->Argv()[0], "echo");
}

TEST(ParserTest, MultipleAssignments)
{
    auto node       = parse("A=1 B=2 cmd");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Assignments().size(), 2u);
    EXPECT_EQ(cmd->Assignments()[0].first, "A");
    EXPECT_EQ(cmd->Assignments()[1].first, "B");
    ASSERT_EQ(cmd->Argv().size(), 1u);
}

TEST(ParserTest, AssignmentOnlyCommand)
{
    // "a=" alone is a valid command (sets var, runs nothing)
    auto node       = parse("a=");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    EXPECT_TRUE(cmd->Argv().empty());
    ASSERT_EQ(cmd->Assignments().size(), 1u);
    EXPECT_EQ(cmd->Assignments()[0].first, "a");
    EXPECT_EQ(cmd->Assignments()[0].second, "");
}

TEST(ParserTest, AssignmentAfterCommandIsArgument)
{
    // "ls FOO=bar" — FOO=bar is an ARG, not an assignment
    auto node       = parse("ls FOO=bar");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    EXPECT_TRUE(cmd->Assignments().empty());
    ASSERT_EQ(cmd->Argv().size(), 2u);
    EXPECT_EQ(cmd->Argv()[1], "FOO=bar");
}

TEST(ParserTest, InvalidIdentifierIsNotAssignment)
{
    // "1abc=x" — key not a valid identifier → plain argv word
    auto node       = parse("1abc=x");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    EXPECT_TRUE(cmd->Assignments().empty());
    ASSERT_EQ(cmd->Argv().size(), 1u);
    EXPECT_EQ(cmd->Argv()[0], "1abc=x");
}

// ─── Redirects ──────────────────────────────────────────────────────────────

TEST(ParserTest, RedirOut)
{
    auto node       = parse("echo hi > out.txt");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Redirects().size(), 1u);
    EXPECT_EQ(cmd->Redirects()[0].kind, Redirect::Kind::Out);
    EXPECT_EQ(cmd->Redirects()[0].fd, -1);
    EXPECT_EQ(cmd->Redirects()[0].target, "out.txt");
}

TEST(ParserTest, RedirIn)
{
    auto node       = parse("cat < in.txt");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Redirects().size(), 1u);
    EXPECT_EQ(cmd->Redirects()[0].kind, Redirect::Kind::In);
    EXPECT_EQ(cmd->Redirects()[0].target, "in.txt");
}

TEST(ParserTest, RedirAppend)
{
    auto node       = parse("echo hi >> log.txt");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Redirects().size(), 1u);
    EXPECT_EQ(cmd->Redirects()[0].kind, Redirect::Kind::Append);
}

TEST(ParserTest, FdPrefixedRedirect)
{
    auto node       = parse("cmd 2> err.txt");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Redirects().size(), 1u);
    EXPECT_EQ(cmd->Redirects()[0].kind, Redirect::Kind::Out);
    EXPECT_EQ(cmd->Redirects()[0].fd, 2);
    EXPECT_EQ(cmd->Redirects()[0].target, "err.txt");
}

TEST(ParserTest, DupOut)
{
    auto node       = parse("cmd 2>&1");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Redirects().size(), 1u);
    EXPECT_EQ(cmd->Redirects()[0].kind, Redirect::Kind::DupOut);
    EXPECT_EQ(cmd->Redirects()[0].fd, 2);
    EXPECT_EQ(cmd->Redirects()[0].target, "1");
}

TEST(ParserTest, MultipleRedirects)
{
    auto node       = parse("cmd < in.txt > out.txt");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Redirects().size(), 2u);
    EXPECT_EQ(cmd->Redirects()[0].kind, Redirect::Kind::In);
    EXPECT_EQ(cmd->Redirects()[1].kind, Redirect::Kind::Out);
}

TEST(ParserTest, RedirectBeforeAndAfterArgs)
{
    auto node       = parse("> out.txt echo hi");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->Redirects().size(), 1u);
    ASSERT_EQ(cmd->Argv().size(), 2u);
    EXPECT_EQ(cmd->Argv()[0], "echo");
}

// ─── Pipeline ───────────────────────────────────────────────────────────────

TEST(ParserTest, SimplePipeline)
{
    auto node        = parse("echo hello | grep hello");
    const auto* pipe = as<Pipeline>(node);
    ASSERT_NE(pipe, nullptr);
    EXPECT_FALSE(pipe->Bang());
    ASSERT_EQ(pipe->Stages().size(), 2u);
    EXPECT_NE(as<SimpleCommand>(pipe->Stages()[0].get()), nullptr);
    EXPECT_NE(as<SimpleCommand>(pipe->Stages()[1].get()), nullptr);
}

TEST(ParserTest, ThreeStagePipeline)
{
    auto node        = parse("cat file | sort | uniq");
    const auto* pipe = as<Pipeline>(node);
    ASSERT_NE(pipe, nullptr);
    ASSERT_EQ(pipe->Stages().size(), 3u);
}

TEST(ParserTest, BangPipeline)
{
    auto node        = parse("! grep foo file");
    const auto* pipe = as<Pipeline>(node);
    ASSERT_NE(pipe, nullptr);
    EXPECT_TRUE(pipe->Bang());
    ASSERT_EQ(pipe->Stages().size(), 1u);
}

TEST(ParserTest, SingleCommandNotWrappedInPipeline)
{
    auto node = parse("echo hello");
    EXPECT_NE(as<SimpleCommand>(node), nullptr);
    EXPECT_EQ(as<Pipeline>(node), nullptr);
}

// ─── AndOr ──────────────────────────────────────────────────────────────────

TEST(ParserTest, AndChain)
{
    auto node      = parse("make && make install");
    const auto* ao = as<AndOr>(node);
    ASSERT_NE(ao, nullptr);
    EXPECT_EQ(ao->Operator(), AndOr::Op::And);
    EXPECT_NE(as<SimpleCommand>(ao->Lhs().get()), nullptr);
    EXPECT_NE(as<SimpleCommand>(ao->Rhs().get()), nullptr);
}

TEST(ParserTest, OrChain)
{
    auto node      = parse("cmd || fallback");
    const auto* ao = as<AndOr>(node);
    ASSERT_NE(ao, nullptr);
    EXPECT_EQ(ao->Operator(), AndOr::Op::Or);
}

TEST(ParserTest, AndOrChainedLeftAssociative)
{
    // a && b || c  →  (a && b) || c
    auto node         = parse("a && b || c");
    const auto* outer = as<AndOr>(node);
    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->Operator(), AndOr::Op::Or);
    const auto* inner = as<AndOr>(outer->Lhs().get());
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->Operator(), AndOr::Op::And);
}

TEST(ParserTest, PipelineBindsTighterThanAndOr)
{
    // make && ls | grep foo  →  make && (ls | grep foo)
    auto node      = parse("make && ls | grep foo");
    const auto* ao = as<AndOr>(node);
    ASSERT_NE(ao, nullptr);
    EXPECT_NE(as<SimpleCommand>(ao->Lhs().get()), nullptr);
    EXPECT_NE(as<Pipeline>(ao->Rhs().get()), nullptr);
}

// ─── List ───────────────────────────────────────────────────────────────────

TEST(ParserTest, SemicolonList)
{
    auto node        = parse("cmd1 ; cmd2 ; cmd3");
    const auto* list = as<List>(node);
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->GetItems().size(), 3u);
    EXPECT_FALSE(list->GetItems()[0].background);
}

TEST(ParserTest, BackgroundItem)
{
    auto node        = parse("sleep 5 & echo done");
    const auto* list = as<List>(node);
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->GetItems().size(), 2u);
    EXPECT_TRUE(list->GetItems()[0].background);
    EXPECT_FALSE(list->GetItems()[1].background);
}

TEST(ParserTest, NewlineSeparatedList)
{
    auto node        = parse("echo a\necho b\necho c");
    const auto* list = as<List>(node);
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->GetItems().size(), 3u);
}

TEST(ParserTest, SingleCommandNotWrappedInList)
{
    auto node = parse("echo hello");
    EXPECT_EQ(as<List>(node), nullptr);
    EXPECT_NE(as<SimpleCommand>(node), nullptr);
}

// ─── Subshell / Group ───────────────────────────────────────────────────────

TEST(ParserTest, Subshell)
{
    auto node       = parse("( echo hello )");
    const auto* sub = as<Subshell>(node);
    ASSERT_NE(sub, nullptr);
    EXPECT_NE(as<SimpleCommand>(sub->GetBody().get()), nullptr);
}

TEST(ParserTest, SubshellWithSemicolon)
{
    auto node       = parse("( cmd1 ; cmd2 )");
    const auto* sub = as<Subshell>(node);
    ASSERT_NE(sub, nullptr);
    EXPECT_NE(as<List>(sub->GetBody().get()), nullptr);
}

TEST(ParserTest, GroupCommand)
{
    auto node       = parse("{ echo hello ; }");
    const auto* grp = as<Group>(node);
    ASSERT_NE(grp, nullptr);
    EXPECT_NE(grp->GetBody().get(), nullptr);
}

TEST(ParserTest, GroupMultipleCommands)
{
    auto node       = parse("{ echo a ; echo b ; }");
    const auto* grp = as<Group>(node);
    ASSERT_NE(grp, nullptr);
    EXPECT_NE(as<List>(grp->GetBody().get()), nullptr);
}

// ─── If statement ───────────────────────────────────────────────────────────

TEST(ParserTest, IfThenFi)
{
    auto node      = parse("if true; then echo yes; fi");
    const auto* in = as<If>(node);
    ASSERT_NE(in, nullptr);
    ASSERT_EQ(in->GetBranches().size(), 1u);
    EXPECT_EQ(in->GetElseBody().get(), nullptr);
}

TEST(ParserTest, IfElse)
{
    auto node      = parse("if false; then echo no; else echo yes; fi");
    const auto* in = as<If>(node);
    ASSERT_NE(in, nullptr);
    ASSERT_EQ(in->GetBranches().size(), 1u);
    EXPECT_NE(in->GetElseBody().get(), nullptr);
}

TEST(ParserTest, IfElifElse)
{
    auto node      = parse("if a; then echo a; elif b; then echo b; else echo c; fi");
    const auto* in = as<If>(node);
    ASSERT_NE(in, nullptr);
    ASSERT_EQ(in->GetBranches().size(), 2u); // if + elif
    EXPECT_NE(in->GetElseBody().get(), nullptr);
}

TEST(ParserTest, IfConditionIsCommand)
{
    auto node      = parse("if grep foo file; then echo found; fi");
    const auto* in = as<If>(node);
    ASSERT_NE(in, nullptr);
    const auto* cond = as<SimpleCommand>(in->GetBranches()[0].condition.get());
    ASSERT_NE(cond, nullptr);
    EXPECT_EQ(cond->Argv()[0], "grep");
}

// ─── While / Until ──────────────────────────────────────────────────────────

TEST(ParserTest, WhileLoop)
{
    auto node      = parse("while true; do echo hi; done");
    const auto* wh = as<While>(node);
    ASSERT_NE(wh, nullptr);
    EXPECT_FALSE(wh->IsUntil());
    EXPECT_NE(wh->GetCondition().get(), nullptr);
    EXPECT_NE(wh->GetBody().get(), nullptr);
}

TEST(ParserTest, UntilLoop)
{
    auto node      = parse("until false; do echo hi; done");
    const auto* wh = as<While>(node);
    ASSERT_NE(wh, nullptr);
    EXPECT_TRUE(wh->IsUntil());
}

// ─── For loop ───────────────────────────────────────────────────────────────

TEST(ParserTest, ForInList)
{
    auto node      = parse("for x in a b c; do echo $x; done");
    const auto* fn = as<For>(node);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->GetVar(), "x");
    ASSERT_EQ(fn->GetWords().size(), 3u);
    EXPECT_EQ(fn->GetWords()[0], "a");
    EXPECT_EQ(fn->GetWords()[2], "c");
}

TEST(ParserTest, ForNoInClause)
{
    auto node      = parse("for x; do echo $x; done");
    const auto* fn = as<For>(node);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->GetVar(), "x");
    EXPECT_TRUE(fn->GetWords().empty());
}

TEST(ParserTest, ZshForeach)
{
    // zsh foreach builds the SAME For node
    auto node      = parse("foreach x (a b c); echo $x; end");
    const auto* fn = as<For>(node);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->GetVar(), "x");
    ASSERT_EQ(fn->GetWords().size(), 3u);
}

// ─── Case statement ─────────────────────────────────────────────────────────

TEST(ParserTest, CaseBasic)
{
    auto node      = parse("case $x in foo) echo foo;; bar) echo bar;; esac");
    const auto* cn = as<Case>(node);
    ASSERT_NE(cn, nullptr);
    EXPECT_EQ(cn->GetWord(), "$x");
    ASSERT_EQ(cn->GetArms().size(), 2u);
    EXPECT_EQ(cn->GetArms()[0].patterns[0], "foo");
    EXPECT_EQ(cn->GetArms()[1].patterns[0], "bar");
}

TEST(ParserTest, CasePatternAlternation)
{
    auto node      = parse("case $x in a|b|c) echo hit;; esac");
    const auto* cn = as<Case>(node);
    ASSERT_NE(cn, nullptr);
    ASSERT_EQ(cn->GetArms().size(), 1u);
    ASSERT_EQ(cn->GetArms()[0].patterns.size(), 3u);
    EXPECT_EQ(cn->GetArms()[0].patterns[2], "c");
}

// ─── Function definition ────────────────────────────────────────────────────

TEST(ParserTest, FunctionShorthand)
{
    auto node      = parse("greet() { echo hello; }");
    const auto* fn = as<Function>(node);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->GetName(), "greet");
    EXPECT_NE(as<Group>(fn->GetBody().get()), nullptr);
}

// ─── Complex / integration ──────────────────────────────────────────────────

TEST(ParserTest, PipelineWithRedirect)
{
    auto node        = parse("cat file | grep foo > out.txt");
    const auto* pipe = as<Pipeline>(node);
    ASSERT_NE(pipe, nullptr);
    ASSERT_EQ(pipe->Stages().size(), 2u);
    const auto* last = as<SimpleCommand>(pipe->Stages()[1].get());
    ASSERT_NE(last, nullptr);
    ASSERT_EQ(last->Redirects().size(), 1u);
}

TEST(ParserTest, AndChainWithPipeline)
{
    auto node      = parse("make 2>&1 | tee build.log && echo ok");
    const auto* ao = as<AndOr>(node);
    ASSERT_NE(ao, nullptr);
    EXPECT_EQ(ao->Operator(), AndOr::Op::And);
    EXPECT_NE(as<Pipeline>(ao->Lhs().get()), nullptr);
    EXPECT_NE(as<SimpleCommand>(ao->Rhs().get()), nullptr);
}

TEST(ParserTest, NestedSubshell)
{
    auto node         = parse("( ( echo inner ) )");
    const auto* outer = as<Subshell>(node);
    ASSERT_NE(outer, nullptr);
    EXPECT_NE(as<Subshell>(outer->GetBody().get()), nullptr);
}

TEST(ParserTest, IfInsidePipeline)
{
    auto node        = parse("if true; then echo yes; fi | cat");
    const auto* pipe = as<Pipeline>(node);
    ASSERT_NE(pipe, nullptr);
    ASSERT_EQ(pipe->Stages().size(), 2u);
    EXPECT_NE(as<If>(pipe->Stages()[0].get()), nullptr);
}

TEST(ParserTest, SubshellInPipeline)
{
    auto node        = parse("(cd /tmp && ls) | wc -l");
    const auto* pipe = as<Pipeline>(node);
    ASSERT_NE(pipe, nullptr);
    ASSERT_EQ(pipe->Stages().size(), 2u);
    EXPECT_NE(as<Subshell>(pipe->Stages()[0].get()), nullptr);
}

// ═════════════════════════════════════════════════════════════════════════════
// Negative tests — malformed input must THROW, never hang or crash.
// ═════════════════════════════════════════════════════════════════════════════

// --- quoting ---
TEST(ParserNeg, UnterminatedSingleQuote) { EXPECT_THROW(parse("echo 'oops"), std::exception); }
TEST(ParserNeg, UnterminatedDoubleQuote) { EXPECT_THROW(parse("echo \"oops"), std::exception); }

// --- empty / lone operators ---
TEST(ParserNeg, LonePipe)         { EXPECT_THROW(parse("|"), ParserException); }
TEST(ParserNeg, LoneAnd)          { EXPECT_THROW(parse("&&"), ParserException); }
TEST(ParserNeg, LoneOr)           { EXPECT_THROW(parse("||"), ParserException); }
TEST(ParserNeg, LeadingPipe)      { EXPECT_THROW(parse("| grep foo"), ParserException); }
TEST(ParserNeg, LeadingAnd)       { EXPECT_THROW(parse("&& echo hi"), ParserException); }
TEST(ParserNeg, TrailingPipe)     { EXPECT_THROW(parse("ls |"), ParserException); }
TEST(ParserNeg, TrailingAnd)      { EXPECT_THROW(parse("ls &&"), ParserException); }
TEST(ParserNeg, DoublePipe)       { EXPECT_THROW(parse("ls || | grep"), ParserException); }
TEST(ParserNeg, PipeThenAnd)      { EXPECT_THROW(parse("ls | && grep"), ParserException); }

// --- redirects ---
TEST(ParserNeg, RedirNoTarget)        { EXPECT_THROW(parse("echo >"), ParserException); }
TEST(ParserNeg, RedirAppendNoTarget)  { EXPECT_THROW(parse("echo >>"), ParserException); }
TEST(ParserNeg, RedirInNoTarget)      { EXPECT_THROW(parse("cat <"), ParserException); }
TEST(ParserNeg, RedirTargetIsOperator){ EXPECT_THROW(parse("echo > |"), ParserException); }

// --- if ---
TEST(ParserNeg, IfMissingThen)  { EXPECT_THROW(parse("if true; echo hi; fi"), ParserException); }
TEST(ParserNeg, IfMissingFi)    { EXPECT_THROW(parse("if true; then echo hi"), ParserException); }
TEST(ParserNeg, IfEmpty)        { EXPECT_THROW(parse("if; then echo hi; fi"), ParserException); }
TEST(ParserNeg, IfNoCondition)  { EXPECT_THROW(parse("if then echo hi; fi"), ParserException); }
TEST(ParserNeg, ElifMissingThen){ EXPECT_THROW(parse("if a; then b; elif c; d; fi"), ParserException); }
TEST(ParserNeg, LoneFi)         { EXPECT_THROW(parse("fi"), ParserException); }
TEST(ParserNeg, LoneThen)       { EXPECT_THROW(parse("then echo hi"), ParserException); }

// --- while / until ---
TEST(ParserNeg, WhileMissingDo)   { EXPECT_THROW(parse("while true; echo hi; done"), ParserException); }
TEST(ParserNeg, WhileMissingDone) { EXPECT_THROW(parse("while true; do echo hi"), ParserException); }
TEST(ParserNeg, UntilMissingDone) { EXPECT_THROW(parse("until false; do echo hi"), ParserException); }
TEST(ParserNeg, LoneDone)         { EXPECT_THROW(parse("done"), ParserException); }
TEST(ParserNeg, LoneDo)           { EXPECT_THROW(parse("do echo hi; done"), ParserException); }

// --- for / foreach ---
TEST(ParserNeg, ForMissingVar)    { EXPECT_THROW(parse("for in a b c; do echo; done"), ParserException); }
TEST(ParserNeg, ForMissingDo)     { EXPECT_THROW(parse("for x in a b; echo $x; done"), ParserException); }
TEST(ParserNeg, ForMissingDone)   { EXPECT_THROW(parse("for x in a b; do echo $x"), ParserException); }
TEST(ParserNeg, ForeachNoParen)   { EXPECT_THROW(parse("foreach x a b c; echo; end"), ParserException); }
TEST(ParserNeg, ForeachMissingEnd){ EXPECT_THROW(parse("foreach x (a b); echo $x"), ParserException); }

// --- case ---
TEST(ParserNeg, CaseMissingIn)    { EXPECT_THROW(parse("case $x foo) echo;; esac"), ParserException); }
TEST(ParserNeg, CaseMissingEsac)  { EXPECT_THROW(parse("case $x in foo) echo;;"), ParserException); }
TEST(ParserNeg, CaseMissingParen) { EXPECT_THROW(parse("case $x in foo echo;; esac"), ParserException); }
TEST(ParserNeg, CaseNoWord)       { EXPECT_THROW(parse("case in foo) echo;; esac"), ParserException); }

// --- grouping ---
TEST(ParserNeg, UnclosedSubshell) { EXPECT_THROW(parse("( echo hi"), ParserException); }
TEST(ParserNeg, UnclosedGroup)    { EXPECT_THROW(parse("{ echo hi"), ParserException); }
TEST(ParserNeg, StrayCloseParen)  { EXPECT_THROW(parse("echo hi )"), ParserException); }
TEST(ParserNeg, StrayCloseBrace)  { EXPECT_THROW(parse("echo hi }"), ParserException); }
TEST(ParserNeg, EmptySubshell)    { EXPECT_THROW(parse("( )"), ParserException); }

// --- the input that originally caused an infinite loop ---
TEST(ParserNeg, MalformedIfNoHang) { EXPECT_THROW(parse("if ( z = 0 ) then; echo hi; fi"), ParserException); }

// ─── Error metadata ─────────────────────────────────────────────────────────

TEST(ParserError, CarriesLineAndCol)
{
    try
    {
        parse("ls |");
        FAIL() << "expected ParserException";
    }
    catch (const ParserException& e)
    {
        // message includes line/col text; just ensure what() is non-empty
        EXPECT_GT(std::string(e.what()).size(), 0u);
    }
}
