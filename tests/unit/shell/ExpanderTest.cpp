// =========================================================
// ExpanderTest — arithmetic expansion $((expr)) inside a word.
// Drives shell::expander::Expand against a real ShellState.
// =========================================================

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "arithmetic/ArithmeticException.hpp"
#include "shell/ShellState.hpp"
#include "shell/expander/Expander.hpp"

namespace
{
std::unique_ptr<shell::ShellState>
makeState(std::map<std::string, std::string> vars = {})
{
    return std::make_unique<shell::ShellState>(std::move(vars));
}

// Expand a single word; expansion currently yields exactly one piece.
std::string expand1(const std::string& word,
                    std::unique_ptr<shell::ShellState>& state)
{
    auto pieces = shell::expander::Expand(word, state);
    return pieces.empty() ? "" : pieces.front();
}
} // namespace

TEST(Expander, NoArithUnchanged)
{
    auto s = makeState();
    EXPECT_EQ(expand1("hello", s), "hello");
}

TEST(Expander, SimpleExpansion)
{
    auto s = makeState();
    EXPECT_EQ(expand1("$((2+3))", s), "5");
}

TEST(Expander, EmbeddedInWord)
{
    auto s = makeState();
    EXPECT_EQ(expand1("a$((1+1))b", s), "a2b");
}

TEST(Expander, MultipleExpansions)
{
    auto s = makeState();
    EXPECT_EQ(expand1("$((1))$((2))", s), "12");
}

TEST(Expander, UsesVariable)
{
    auto s = makeState({{"i", "4"}});
    EXPECT_EQ(expand1("$((i*2))", s), "8");
}

TEST(Expander, DollarVarInsideArith)
{
    auto s = makeState({{"i", "4"}});
    EXPECT_EQ(expand1("$((i + 1))", s), "5");
}

TEST(Expander, NestedParens)
{
    auto s = makeState();
    EXPECT_EQ(expand1("$((2*(3+4)))", s), "14");
}

TEST(Expander, AssignmentSideEffect)
{
    auto s = makeState();
    expand1("$((x = 7))", s);
    auto v = s->GetVar("x");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "7");
}

TEST(Expander, MalformedThrows)
{
    auto s = makeState();
    EXPECT_THROW(expand1("$((1/0))", s), arithmetic::ArithmeticException);
}

TEST(Expander, UnterminatedThrows)
{
    auto s = makeState();
    EXPECT_THROW(expand1("$((2+3", s), arithmetic::ArithmeticException);
}
