// =========================================================
// ExpanderTest — arithmetic expansion $((expr)) inside a word.
// Drives shell::expander::Expand against a real ShellState.
// =========================================================

#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

#include "arithmetic/ArithmeticException.hpp"
#include "parser/ParserException.hpp"
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

// ─── $VAR parameter expansion ────────────────────────────────────────────────
TEST(Expander, DollarVar)
{
    auto s = makeState({{"x", "hello"}});
    EXPECT_EQ(expand1("$x", s), "hello");
}

TEST(Expander, DollarVarUnsetIsEmpty)
{
    auto s = makeState();
    EXPECT_EQ(expand1("$nope", s), "");
}

TEST(Expander, DollarVarEmbedded)
{
    auto s = makeState({{"USER", "cj"}});
    EXPECT_EQ(expand1("/home/$USER/bin", s), "/home/cj/bin");
}

TEST(Expander, DollarVarStopsAtNonNameChar)
{
    auto s = makeState({{"x", "5"}});
    EXPECT_EQ(expand1("$x.txt", s), "5.txt"); // '.' ends the name
}

TEST(Expander, DollarDigitIsLiteral)
{
    auto s = makeState();
    EXPECT_EQ(expand1("$1", s), "$1"); // positional params not supported -> literal
}

// ─── ${VAR} braced parameter expansion ───────────────────────────────────────
TEST(Expander, BracedVar)
{
    auto s = makeState({{"HOME", "/root"}});
    EXPECT_EQ(expand1("${HOME}", s), "/root");
}

TEST(Expander, BracedVarButtsAgainstText)
{
    auto s = makeState({{"a", "5"}});
    EXPECT_EQ(expand1("${a}_done", s), "5_done"); // braces let value butt up to text
}

TEST(Expander, BracedUnsetIsEmpty)
{
    auto s = makeState();
    EXPECT_EQ(expand1("${nope}", s), "");
}

TEST(Expander, BracedUnterminatedThrows)
{
    auto s = makeState();
    EXPECT_THROW(expand1("${a", s), parser::ParserException);
}

// ─── glob / pathname expansion ───────────────────────────────────────────────
TEST(Expander, GlobNoMatchStaysLiteral)
{
    auto s = makeState();
    auto pieces = shell::expander::Expand("*.no_such_ext_xyz", s);
    ASSERT_EQ(pieces.size(), 1u);
    EXPECT_EQ(pieces.front(), "*.no_such_ext_xyz"); // bash default: unmatched -> literal
}

TEST(Expander, GlobMatchesFiles)
{
    char templ[] = "/tmp/shell_glob_XXXXXX";
    char* dir = ::mkdtemp(templ);
    ASSERT_NE(dir, nullptr);
    const std::string base = dir;
    std::ofstream(base + "/a.txt") << "x";
    std::ofstream(base + "/b.txt") << "x";
    std::ofstream(base + "/c.log") << "x";

    char prev[4096];
    ASSERT_NE(::getcwd(prev, sizeof prev), nullptr);
    ASSERT_EQ(::chdir(dir), 0);

    auto s      = makeState();
    auto pieces = shell::expander::Expand("*.txt", s);

    ASSERT_EQ(::chdir(prev), 0); // restore before asserting

    std::sort(pieces.begin(), pieces.end());
    ASSERT_EQ(pieces.size(), 2u);
    EXPECT_EQ(pieces[0], "a.txt");
    EXPECT_EQ(pieces[1], "b.txt");

    ::unlink((base + "/a.txt").c_str());
    ::unlink((base + "/b.txt").c_str());
    ::unlink((base + "/c.log").c_str());
    ::rmdir(dir);
}
