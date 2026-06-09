// =========================================================
// ArithmeticCommandTest — integration: parse "((expr))", run it through the
// Executor, check exit status and variable side-effects via ShellState.
// Exercises tokenizer raw-capture -> parser -> ArithmeticCommand ->
// Executor -> ShellArithmeticVars adapter -> engine.
// =========================================================

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>

#include "builtins/BuiltinDispatcher.hpp"
#include "exec/Executor.hpp"
#include "parser/Parser.hpp"
#include "parser/Tokenizer.hpp"
#include "shell/ShellState.hpp"

namespace
{
// Run src against the given state; returns the exit status.
int runOn(const std::string& src, std::unique_ptr<shell::ShellState>& state)
{
    auto disp = std::make_unique<builtins::BuiltinDispatcher>();
    exec::Executor exec(state, disp);

    parser::Tokenizer tok(src);
    parser::Parser    p(tok.Tokenize());
    auto              tree = p.Parse();
    return exec.Run(tree);
}

std::unique_ptr<shell::ShellState> makeState(std::map<std::string, std::string> vars = {})
{
    return std::make_unique<shell::ShellState>(std::move(vars));
}
} // namespace

// ─── Exit status (bash rule: result != 0 -> 0, result == 0 -> 1) ─────────────
TEST(ArithmeticCommand, TrueComparisonStatusZero)
{
    auto state = makeState();
    EXPECT_EQ(runOn("((2 < 3))", state), 0);
}

TEST(ArithmeticCommand, FalseComparisonStatusOne)
{
    auto state = makeState();
    EXPECT_EQ(runOn("((5 < 3))", state), 1);
}

TEST(ArithmeticCommand, NonZeroResultStatusZero)
{
    auto state = makeState();
    EXPECT_EQ(runOn("((2 + 3))", state), 0);
}

TEST(ArithmeticCommand, ZeroResultStatusOne)
{
    auto state = makeState();
    EXPECT_EQ(runOn("((5 - 5))", state), 1);
}

// ─── Side effects ────────────────────────────────────────────────────────────
TEST(ArithmeticCommand, AssignmentSetsVar)
{
    auto state = makeState();
    runOn("((i = 5))", state);
    auto v = state->GetVar("i");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "5");
}

TEST(ArithmeticCommand, PostIncrementMutatesVar)
{
    auto state = makeState({{"i", "5"}});
    EXPECT_EQ(runOn("((i++))", state), 0);   // returns old (5) -> status 0
    auto v = state->GetVar("i");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "6");
}

TEST(ArithmeticCommand, CompoundAssignMutatesVar)
{
    auto state = makeState({{"sum", "10"}});
    runOn("((sum += 5))", state);
    auto v = state->GetVar("sum");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "15");
}

TEST(ArithmeticCommand, UsesExistingVar)
{
    auto state = makeState({{"i", "4"}});
    runOn("((j = i * 2))", state);
    auto v = state->GetVar("j");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "8");
}

// ─── Malformed expression: status 1, no crash ────────────────────────────────
TEST(ArithmeticCommand, MalformedIsStatusOneNotCrash)
{
    auto state = makeState();
    EXPECT_EQ(runOn("((1 / 0))", state), 1);
}

// ─── $((expr)) expansion reaching the command (argv / assignment) ────────────
TEST(ArithmeticCommand, AssignmentValueExpanded)
{
    auto state = makeState();
    runOn("i=$((2+2))", state);          // expansion in the assignment value
    auto v = state->GetVar("i");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "4");
}
