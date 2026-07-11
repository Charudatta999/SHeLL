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

#include <pwd.h>
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

// Stub: command substitution is exercised at the REPL level, not here.
const shell::expander::CommandRunner stubRunner =
    [](const std::string&) { return std::string{}; };

// Expand a single word; expansion currently yields exactly one piece.
std::string expand1(const std::string& word,
                    std::unique_ptr<shell::ShellState>& state)
{
    auto pieces = shell::expander::Expand(word, state, stubRunner);
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
    auto pieces = shell::expander::Expand("*.no_such_ext_xyz", s, stubRunner);
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
    auto pieces = shell::expander::Expand("*.txt", s, stubRunner);

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

// ─── command substitution $( ) — splicing via the CommandRunner callback ─────
TEST(Expander, CommandSubSplicesRunnerOutput)
{
    auto s = makeState();
    shell::expander::CommandRunner fake =
        [](const std::string&) { return std::string{"hi"}; };
    auto pieces = shell::expander::Expand("$(x)", s, fake);
    ASSERT_EQ(pieces.size(), 1u);
    EXPECT_EQ(pieces.front(), "hi");
}

TEST(Expander, CommandSubEmbeddedInWord)
{
    auto s = makeState();
    shell::expander::CommandRunner fake =
        [](const std::string&) { return std::string{"hi"}; };
    auto pieces = shell::expander::Expand("a$(x)b", s, fake);
    ASSERT_EQ(pieces.size(), 1u);
    EXPECT_EQ(pieces.front(), "ahib");
}

TEST(Expander, CommandSubPassesInnerText)
{
    auto s = makeState();
    std::string seen;
    shell::expander::CommandRunner fake =
        [&seen](const std::string& text)
    {
        seen = text;
        return std::string{};
    };
    shell::expander::Expand("$(echo a b)", s, fake);
    EXPECT_EQ(seen, "echo a b");
}

TEST(Expander, CommandSubNestedParensKeptWhole)
{
    auto s = makeState();
    std::string seen;
    shell::expander::CommandRunner fake =
        [&seen](const std::string& text)
    {
        seen = text;
        return std::string{};
    };
    shell::expander::Expand("$(echo (nested))", s, fake);
    EXPECT_EQ(seen, "echo (nested)"); // inner parens balanced, kept
}

TEST(Expander, CommandSubUnterminatedThrows)
{
    auto s = makeState();
    EXPECT_THROW(expand1("$(foo", s), parser::ParserException);
}

// ─── special parameters $? and $$ ────────────────────────────────────────────
TEST(Expander, LastExitCodeExpands)
{
    auto s = makeState();
    s->SetLastCommandExitCode(7);
    EXPECT_EQ(expand1("$?", s), "7");
}

TEST(Expander, LastExitCodeDefaultsZero)
{
    auto s = makeState();
    EXPECT_EQ(expand1("$?", s), "0");
}

TEST(Expander, ShellPidExpands)
{
    auto s = makeState();
    EXPECT_EQ(expand1("$$", s), std::to_string(s->GetShellPid()));
}

TEST(Expander, ExitCodeEmbeddedInWord)
{
    auto s = makeState();
    s->SetLastCommandExitCode(3);
    EXPECT_EQ(expand1("rc=$?.", s), "rc=3.");
}

// ─── brace expansion (textual, stateless) ────────────────────────────────────
namespace
{
using Words = std::vector<std::string>;
Words brace(const std::string& word)
{
    return shell::expander::BraceExpand(word);
}
} // namespace

TEST(BraceExpand, NoBracesUnchanged)
{
    EXPECT_EQ(brace("hello"), (Words{"hello"}));
}

TEST(BraceExpand, EmptyStringUnchanged)
{
    EXPECT_EQ(brace(""), (Words{""}));
}

TEST(BraceExpand, SimpleCommaList)
{
    EXPECT_EQ(brace("{a,b,c}"), (Words{"a", "b", "c"}));
}

TEST(BraceExpand, ListWithPreambleAndPostscript)
{
    EXPECT_EQ(brace("a{b,c}d"), (Words{"abd", "acd"}));
}

TEST(BraceExpand, EmptyAlternative)
{
    EXPECT_EQ(brace("a{,b}c"), (Words{"ac", "abc"}));
}

TEST(BraceExpand, CrossProductOfAdjacentGroups)
{
    EXPECT_EQ(brace("{a,b}{1,2}"),
              (Words{"a1", "a2", "b1", "b2"}));
}

TEST(BraceExpand, Nested)
{
    EXPECT_EQ(brace("{a,b{c,d}}"), (Words{"a", "bc", "bd"}));
}

TEST(BraceExpand, NestedCommaIsScoped)
{
    EXPECT_EQ(brace("{a,b{c,d}e}"), (Words{"a", "bce", "bde"}));
}

TEST(BraceExpand, NumericRange)
{
    EXPECT_EQ(brace("{1..5}"), (Words{"1", "2", "3", "4", "5"}));
}

TEST(BraceExpand, NumericRangeDescending)
{
    EXPECT_EQ(brace("{5..1}"), (Words{"5", "4", "3", "2", "1"}));
}

TEST(BraceExpand, NumericRangeWithStep)
{
    EXPECT_EQ(brace("{1..9..2}"),
              (Words{"1", "3", "5", "7", "9"}));
}

TEST(BraceExpand, NumericRangeNegative)
{
    EXPECT_EQ(brace("{-2..2}"),
              (Words{"-2", "-1", "0", "1", "2"}));
}

TEST(BraceExpand, ZeroPaddedRange)
{
    EXPECT_EQ(brace("{01..10}"),
              (Words{"01", "02", "03", "04", "05", "06", "07", "08",
                     "09", "10"}));
}

TEST(BraceExpand, ZeroPaddedWidthFromWidestOperand)
{
    EXPECT_EQ(brace("{001..10}"),
              (Words{"001", "002", "003", "004", "005", "006", "007",
                     "008", "009", "010"}));
}

TEST(BraceExpand, PlainZeroNotPadded)
{
    EXPECT_EQ(brace("{0..10}"),
              (Words{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                     "10"}));
}

TEST(BraceExpand, CharRange)
{
    EXPECT_EQ(brace("{a..e}"),
              (Words{"a", "b", "c", "d", "e"}));
}

TEST(BraceExpand, CharRangeDescending)
{
    EXPECT_EQ(brace("{e..a}"),
              (Words{"e", "d", "c", "b", "a"}));
}

TEST(BraceExpand, CharRangeWithStep)
{
    EXPECT_EQ(brace("{a..g..2}"), (Words{"a", "c", "e", "g"}));
}

TEST(BraceExpand, PassthroughNoCommaOrRange)
{
    EXPECT_EQ(brace("{abc}"), (Words{"{abc}"}));
}

TEST(BraceExpand, PassthroughEmptyBraces)
{
    EXPECT_EQ(brace("{}"), (Words{"{}"}));
}

TEST(BraceExpand, PassthroughBadRange)
{
    EXPECT_EQ(brace("{1..x}"), (Words{"{1..x}"}));
}

TEST(BraceExpand, PassthroughUnbalanced)
{
    EXPECT_EQ(brace("{a,b"), (Words{"{a,b"}));
}

TEST(BraceExpand, PassthroughFollowedByValidGroup)
{
    EXPECT_EQ(brace("{abc}{d,e}"),
              (Words{"{abc}d", "{abc}e"}));
}

TEST(BraceExpand, SingleAlternativeNoComma)
{
    EXPECT_EQ(brace("{a}"), (Words{"{a}"}));
}

// ─── tilde expansion ─────────────────────────────────────────────────────────
TEST(Tilde, AloneExpandsHome)
{
    auto s = makeState({{"HOME", "/root"}});
    EXPECT_EQ(expand1("~", s), "/root");
}

TEST(Tilde, LeadingPathExpands)
{
    auto s = makeState({{"HOME", "/root"}});
    EXPECT_EQ(expand1("~/bin", s), "/root/bin");
}

TEST(Tilde, MidWordIsLiteral)
{
    auto s = makeState({{"HOME", "/root"}});
    EXPECT_EQ(expand1("a~b", s), "a~b");
}

TEST(Tilde, TrailingTildeIsLiteral)
{
    auto s = makeState({{"HOME", "/root"}});
    EXPECT_EQ(expand1("x~", s), "x~");
}

TEST(Tilde, HomeUnsetFallsBackToPasswd)
{
    auto s = makeState();
    const passwd* pw = ::getpwuid(::getuid());
    ASSERT_NE(pw, nullptr);
    EXPECT_EQ(expand1("~", s), pw->pw_dir);
}

TEST(Tilde, NamedUserExpandsViaGetpwnam)
{
    auto s = makeState();
    const passwd* pw = ::getpwuid(::getuid());
    ASSERT_NE(pw, nullptr);
    EXPECT_EQ(expand1("~" + std::string(pw->pw_name), s),
              pw->pw_dir);
}

TEST(Tilde, UnknownUserStaysLiteral)
{
    auto s = makeState({{"HOME", "/root"}});
    EXPECT_EQ(expand1("~no_such_user_xyz/f", s),
              "~no_such_user_xyz/f");
}

TEST(Tilde, PlusExpandsPwd)
{
    auto s = makeState({{"PWD", "/somewhere"}});
    EXPECT_EQ(expand1("~+/x", s), "/somewhere/x");
}

TEST(Tilde, PlusWithoutPwdVarUsesShellCwd)
{
    auto s = makeState();
    EXPECT_EQ(expand1("~+", s), s->GetCWD());
}

TEST(Tilde, MinusExpandsOldpwd)
{
    auto s = makeState({{"OLDPWD", "/prev"}});
    EXPECT_EQ(expand1("~-", s), "/prev");
}

TEST(Tilde, MinusWithoutOldpwdStaysLiteral)
{
    auto s = makeState();
    EXPECT_EQ(expand1("~-", s), "~-");
}

TEST(Tilde, ColonNotSpecialInNormalWord)
{
    auto s = makeState({{"HOME", "/root"}});
    EXPECT_EQ(expand1("a:~/b", s), "a:~/b");
}

TEST(Tilde, AssignmentExpandsAfterColons)
{
    auto s = makeState({{"HOME", "/root"}});
    auto pieces = shell::expander::Expand("~/bin:~/sbin:/usr/bin",
                                          s,
                                          stubRunner,
                                          true);
    ASSERT_EQ(pieces.size(), 1u);
    EXPECT_EQ(pieces.front(), "/root/bin:/root/sbin:/usr/bin");
}

TEST(Tilde, AssignmentUnknownUserSegmentStaysLiteral)
{
    auto s = makeState({{"HOME", "/root"}});
    auto pieces = shell::expander::Expand("~/bin:~no_such_user_xyz",
                                          s,
                                          stubRunner,
                                          true);
    ASSERT_EQ(pieces.size(), 1u);
    EXPECT_EQ(pieces.front(), "/root/bin:~no_such_user_xyz");
}

TEST(Tilde, ResultIsNotRescanned)
{
    auto s = makeState({{"HOME", "/ro$x"}, {"x", "boom"}});
    EXPECT_EQ(expand1("~", s), "/ro$x"); // bash: tilde result is verbatim
}

TEST(Tilde, DollarAfterFailedTildeStillExpands)
{
    auto s = makeState({{"USER", "cj"}});
    // bash: no user literally named "$USER" -> tilde stays, $USER expands
    EXPECT_EQ(expand1("~$USER", s), "~cj");
}
