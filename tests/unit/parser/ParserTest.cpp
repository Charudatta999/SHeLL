#include <gtest/gtest.h>
#include <string>
#include <memory>

#include "parser/Tokenizer.hpp"
#include "parser/Parser.hpp"
#include "parser/Command.hpp"

using namespace parser;

// ─── Helpers ──────────────────────────────────────────────────────────────────

static AstNodePtr parse(const std::string& input)
{
    Tokenizer tok(input);
    Parser    p(tok.tokenize());
    return p.parse();
}

template<typename T>
static const T* as(const AstNodePtr& node)
{
    return dynamic_cast<const T*>(node.get());
}

// ─── SimpleCommand ────────────────────────────────────────────────────────────

TEST(ParserTest, SingleWord)
{
    auto node = parse("echo");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->argv.size(), 1u);
    EXPECT_EQ(cmd->argv[0], "echo");
}

TEST(ParserTest, MultipleWords)
{
    auto node = parse("echo hello world");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->argv.size(), 3u);
    EXPECT_EQ(cmd->argv[0], "echo");
    EXPECT_EQ(cmd->argv[1], "hello");
    EXPECT_EQ(cmd->argv[2], "world");
}

TEST(ParserTest, SingleQuotedArg)
{
    auto node = parse("echo 'hello world'");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->argv.size(), 2u);
    EXPECT_EQ(cmd->argv[1], "hello world");
}

TEST(ParserTest, DoubleQuotedArg)
{
    auto node = parse("echo \"hello world\"");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->argv.size(), 2u);
    EXPECT_EQ(cmd->argv[1], "hello world");
}

TEST(ParserTest, LeadingAssignment)
{
    auto node = parse("FOO=bar echo hello");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->assignments.size(), 1u);
    EXPECT_EQ(cmd->assignments[0].first,  "FOO");
    EXPECT_EQ(cmd->assignments[0].second, "bar");
    ASSERT_EQ(cmd->argv.size(), 2u);
    EXPECT_EQ(cmd->argv[0], "echo");
}

TEST(ParserTest, MultipleAssignments)
{
    auto node = parse("A=1 B=2 cmd");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->assignments.size(), 2u);
    EXPECT_EQ(cmd->assignments[0].first, "A");
    EXPECT_EQ(cmd->assignments[1].first, "B");
    ASSERT_EQ(cmd->argv.size(), 1u);
}

TEST(ParserTest, RedirOut)
{
    auto node = parse("echo hi > out.txt");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->redirects.size(), 1u);
    EXPECT_EQ(cmd->redirects[0].kind,   Redirect::Kind::Out);
    EXPECT_EQ(cmd->redirects[0].fd,     -1);
    EXPECT_EQ(cmd->redirects[0].target, "out.txt");
}

TEST(ParserTest, RedirIn)
{
    auto node = parse("cat < in.txt");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->redirects.size(), 1u);
    EXPECT_EQ(cmd->redirects[0].kind,   Redirect::Kind::In);
    EXPECT_EQ(cmd->redirects[0].target, "in.txt");
}

TEST(ParserTest, RedirAppend)
{
    auto node = parse("echo hi >> log.txt");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->redirects.size(), 1u);
    EXPECT_EQ(cmd->redirects[0].kind, Redirect::Kind::Append);
}

TEST(ParserTest, FdPrefixedRedirect)
{
    auto node = parse("cmd 2> err.txt");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->redirects.size(), 1u);
    EXPECT_EQ(cmd->redirects[0].kind,   Redirect::Kind::Out);
    EXPECT_EQ(cmd->redirects[0].fd,     2);
    EXPECT_EQ(cmd->redirects[0].target, "err.txt");
}

TEST(ParserTest, DupOut)
{
    auto node = parse("cmd 2>&1");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->redirects.size(), 1u);
    EXPECT_EQ(cmd->redirects[0].kind, Redirect::Kind::DupOut);
    EXPECT_EQ(cmd->redirects[0].fd,   2);
    EXPECT_EQ(cmd->redirects[0].target, "1");
}

TEST(ParserTest, MultipleRedirects)
{
    auto node = parse("cmd < in.txt > out.txt");
    const auto* cmd = as<SimpleCommand>(node);
    ASSERT_NE(cmd, nullptr);
    ASSERT_EQ(cmd->redirects.size(), 2u);
    EXPECT_EQ(cmd->redirects[0].kind, Redirect::Kind::In);
    EXPECT_EQ(cmd->redirects[1].kind, Redirect::Kind::Out);
}

// ─── Pipeline ─────────────────────────────────────────────────────────────────

TEST(ParserTest, SimplePipeline)
{
    auto node = parse("echo hello | grep hello");
    const auto* pipe = as<PipelineNode>(node);
    ASSERT_NE(pipe, nullptr);
    EXPECT_FALSE(pipe->bang);
    ASSERT_EQ(pipe->stages.size(), 2u);

    const auto* s0 = as<SimpleCommand>(pipe->stages[0]);
    ASSERT_NE(s0, nullptr);
    EXPECT_EQ(s0->argv[0], "echo");

    const auto* s1 = as<SimpleCommand>(pipe->stages[1]);
    ASSERT_NE(s1, nullptr);
    EXPECT_EQ(s1->argv[0], "grep");
}

TEST(ParserTest, ThreeStagePipeline)
{
    auto node = parse("cat file | sort | uniq");
    const auto* pipe = as<PipelineNode>(node);
    ASSERT_NE(pipe, nullptr);
    ASSERT_EQ(pipe->stages.size(), 3u);
}

TEST(ParserTest, BangPipeline)
{
    auto node = parse("! grep foo file");
    const auto* pipe = as<PipelineNode>(node);
    ASSERT_NE(pipe, nullptr);
    EXPECT_TRUE(pipe->bang);
    ASSERT_EQ(pipe->stages.size(), 1u);
}

TEST(ParserTest, SingleCommandNotWrappedInPipeline)
{
    // Single stage, no bang — should unwrap to SimpleCommand directly
    auto node = parse("echo hello");
    EXPECT_NE(as<SimpleCommand>(node), nullptr);
    EXPECT_EQ(as<PipelineNode>(node), nullptr);
}

// ─── AndOr ────────────────────────────────────────────────────────────────────

TEST(ParserTest, AndChain)
{
    auto node = parse("make && make install");
    const auto* ao = as<AndOrNode>(node);
    ASSERT_NE(ao, nullptr);
    EXPECT_EQ(ao->op, AndOrNode::Op::And);
    EXPECT_NE(as<SimpleCommand>(ao->lhs), nullptr);
    EXPECT_NE(as<SimpleCommand>(ao->rhs), nullptr);
}

TEST(ParserTest, OrChain)
{
    auto node = parse("cmd || fallback");
    const auto* ao = as<AndOrNode>(node);
    ASSERT_NE(ao, nullptr);
    EXPECT_EQ(ao->op, AndOrNode::Op::Or);
}

TEST(ParserTest, AndOrChained)
{
    // a && b || c  →  (a && b) || c  (left-associative)
    auto node = parse("a && b || c");
    const auto* outer = as<AndOrNode>(node);
    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->op, AndOrNode::Op::Or);

    const auto* inner = as<AndOrNode>(outer->lhs);
    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->op, AndOrNode::Op::And);
}

// ─── List ─────────────────────────────────────────────────────────────────────

TEST(ParserTest, SemicolonList)
{
    auto node = parse("cmd1 ; cmd2 ; cmd3");
    const auto* list = as<ListNode>(node);
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->items.size(), 3u);
    EXPECT_FALSE(list->items[0].background);
    EXPECT_FALSE(list->items[1].background);
    EXPECT_FALSE(list->items[2].background);
}

TEST(ParserTest, BackgroundItem)
{
    auto node = parse("sleep 5 & echo done");
    const auto* list = as<ListNode>(node);
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->items.size(), 2u);
    EXPECT_TRUE(list->items[0].background);
    EXPECT_FALSE(list->items[1].background);
}

TEST(ParserTest, NewlineSeparatedList)
{
    auto node = parse("echo a\necho b\necho c");
    const auto* list = as<ListNode>(node);
    ASSERT_NE(list, nullptr);
    ASSERT_EQ(list->items.size(), 3u);
}

TEST(ParserTest, SingleCommandNotWrappedInList)
{
    // Single item, no background — should unwrap
    auto node = parse("echo hello");
    EXPECT_EQ(as<ListNode>(node), nullptr);
    EXPECT_NE(as<SimpleCommand>(node), nullptr);
}

// ─── Subshell ─────────────────────────────────────────────────────────────────

TEST(ParserTest, Subshell)
{
    auto node = parse("( echo hello )");
    const auto* sub = as<SubshellNode>(node);
    ASSERT_NE(sub, nullptr);
    EXPECT_NE(as<SimpleCommand>(sub->body), nullptr);
}

TEST(ParserTest, SubshellWithSemicolon)
{
    auto node = parse("( cmd1 ; cmd2 )");
    const auto* sub = as<SubshellNode>(node);
    ASSERT_NE(sub, nullptr);
    EXPECT_NE(as<ListNode>(sub->body), nullptr);
}

// ─── Group ────────────────────────────────────────────────────────────────────

TEST(ParserTest, GroupCommand)
{
    auto node = parse("{ echo hello ; }");
    const auto* grp = as<GroupNode>(node);
    ASSERT_NE(grp, nullptr);
    EXPECT_NE(as<SimpleCommand>(grp->body), nullptr);
}

// ─── If statement ─────────────────────────────────────────────────────────────

TEST(ParserTest, IfThenFi)
{
    auto node = parse("if true; then echo yes; fi");
    const auto* ifn = as<IfNode>(node);
    ASSERT_NE(ifn, nullptr);
    ASSERT_EQ(ifn->branches.size(), 1u);
    EXPECT_EQ(ifn->else_body, nullptr);
}

TEST(ParserTest, IfElse)
{
    auto node = parse("if false; then echo no; else echo yes; fi");
    const auto* ifn = as<IfNode>(node);
    ASSERT_NE(ifn, nullptr);
    ASSERT_EQ(ifn->branches.size(), 1u);
    EXPECT_NE(ifn->else_body, nullptr);
}

TEST(ParserTest, IfElifElse)
{
    auto node = parse("if a; then echo a; elif b; then echo b; else echo c; fi");
    const auto* ifn = as<IfNode>(node);
    ASSERT_NE(ifn, nullptr);
    ASSERT_EQ(ifn->branches.size(), 2u);  // if + elif
    EXPECT_NE(ifn->else_body, nullptr);
}

TEST(ParserTest, IfConditionIsCommand)
{
    auto node = parse("if grep foo file; then echo found; fi");
    const auto* ifn = as<IfNode>(node);
    ASSERT_NE(ifn, nullptr);
    const auto* cond = as<SimpleCommand>(ifn->branches[0].condition);
    ASSERT_NE(cond, nullptr);
    EXPECT_EQ(cond->argv[0], "grep");
}

// ─── While / Until ────────────────────────────────────────────────────────────

TEST(ParserTest, WhileLoop)
{
    auto node = parse("while true; do echo hi; done");
    const auto* wh = as<WhileNode>(node);
    ASSERT_NE(wh, nullptr);
    EXPECT_FALSE(wh->until);
    EXPECT_NE(wh->condition, nullptr);
    EXPECT_NE(wh->body, nullptr);
}

TEST(ParserTest, UntilLoop)
{
    auto node = parse("until false; do echo hi; done");
    const auto* wh = as<WhileNode>(node);
    ASSERT_NE(wh, nullptr);
    EXPECT_TRUE(wh->until);
}

// ─── For loop ─────────────────────────────────────────────────────────────────

TEST(ParserTest, ForInList)
{
    auto node = parse("for x in a b c; do echo $x; done");
    const auto* fn = as<ForNode>(node);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->var, "x");
    ASSERT_EQ(fn->words.size(), 3u);
    EXPECT_EQ(fn->words[0], "a");
    EXPECT_EQ(fn->words[1], "b");
    EXPECT_EQ(fn->words[2], "c");
    EXPECT_NE(fn->body, nullptr);
}

TEST(ParserTest, ForNoInClause)
{
    auto node = parse("for x; do echo $x; done");
    const auto* fn = as<ForNode>(node);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->var, "x");
    EXPECT_TRUE(fn->words.empty()); // iterates over "$@"
}

// ─── Case statement ───────────────────────────────────────────────────────────

TEST(ParserTest, CaseBasic)
{
    auto node = parse("case $x in foo) echo foo;; bar) echo bar;; esac");
    const auto* cn = as<CaseNode>(node);
    ASSERT_NE(cn, nullptr);
    EXPECT_EQ(cn->word, "$x");
    ASSERT_EQ(cn->arms.size(), 2u);
    EXPECT_EQ(cn->arms[0].patterns[0], "foo");
    EXPECT_EQ(cn->arms[1].patterns[0], "bar");
}

TEST(ParserTest, CasePatternAlternation)
{
    auto node = parse("case $x in a|b|c) echo hit;; esac");
    const auto* cn = as<CaseNode>(node);
    ASSERT_NE(cn, nullptr);
    ASSERT_EQ(cn->arms.size(), 1u);
    ASSERT_EQ(cn->arms[0].patterns.size(), 3u);
    EXPECT_EQ(cn->arms[0].patterns[0], "a");
    EXPECT_EQ(cn->arms[0].patterns[1], "b");
    EXPECT_EQ(cn->arms[0].patterns[2], "c");
}

// ─── Function definition ──────────────────────────────────────────────────────

TEST(ParserTest, FunctionShorthand)
{
    auto node = parse("greet() { echo hello; }");
    const auto* fn = as<FunctionNode>(node);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->name, "greet");
    EXPECT_NE(as<GroupNode>(fn->body), nullptr);
}

TEST(ParserTest, FunctionKeyword)
{
    auto node = parse("function greet { echo hello; }");
    const auto* fn = as<FunctionNode>(node);
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->name, "greet");
}

// ─── Complex / integration ────────────────────────────────────────────────────

TEST(ParserTest, PipelineWithRedirect)
{
    auto node = parse("cat file | grep foo > out.txt");
    const auto* pipe = as<PipelineNode>(node);
    ASSERT_NE(pipe, nullptr);
    ASSERT_EQ(pipe->stages.size(), 2u);

    // redirect lives on the last stage
    const auto* last = as<SimpleCommand>(pipe->stages[1]);
    ASSERT_NE(last, nullptr);
    ASSERT_EQ(last->redirects.size(), 1u);
    EXPECT_EQ(last->redirects[0].kind, Redirect::Kind::Out);
}

TEST(ParserTest, AndChainWithPipeline)
{
    auto node = parse("make 2>&1 | tee build.log && echo ok");
    const auto* ao = as<AndOrNode>(node);
    ASSERT_NE(ao, nullptr);
    EXPECT_EQ(ao->op, AndOrNode::Op::And);
    EXPECT_NE(as<PipelineNode>(ao->lhs), nullptr);
    EXPECT_NE(as<SimpleCommand>(ao->rhs), nullptr);
}

TEST(ParserTest, NestedSubshell)
{
    auto node = parse("( ( echo inner ) )");
    const auto* outer = as<SubshellNode>(node);
    ASSERT_NE(outer, nullptr);
    const auto* inner = as<SubshellNode>(outer->body);
    ASSERT_NE(inner, nullptr);
}

TEST(ParserTest, IfInsidePipeline)
{
    auto node = parse("if true; then echo yes; fi | cat");
    const auto* pipe = as<PipelineNode>(node);
    ASSERT_NE(pipe, nullptr);
    ASSERT_EQ(pipe->stages.size(), 2u);
    EXPECT_NE(as<IfNode>(pipe->stages[0]), nullptr);
    EXPECT_NE(as<SimpleCommand>(pipe->stages[1]), nullptr);
}

// ─── Error handling ───────────────────────────────────────────────────────────

TEST(ParserTest, UnterminatedSingleQuoteThrows)
{
    EXPECT_THROW(parse("echo 'unterminated"), std::runtime_error);
}

TEST(ParserTest, UnterminatedDoubleQuoteThrows)
{
    EXPECT_THROW(parse("echo \"unterminated"), std::runtime_error);
}

TEST(ParserTest, MissingFiThrows)
{
    EXPECT_THROW(parse("if true; then echo hi"), ParseError);
}

TEST(ParserTest, MissingDoneThrows)
{
    EXPECT_THROW(parse("while true; do echo hi"), ParseError);
}

TEST(ParserTest, MissingThenThrows)
{
    EXPECT_THROW(parse("if true; echo hi; fi"), ParseError);
}

TEST(ParserTest, ParseErrorCarriesLineInfo)
{
    try {
        parse("if true; echo hi; fi");
        FAIL() << "expected ParseError";
    } catch (const ParseError& e) {
        EXPECT_GT(e.line(), 0);
        EXPECT_GT(e.col(),  0);
    }
}
