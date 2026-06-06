// =========================================================
// ExecutorTest — light integration: parse a line, run it, check exit code.
// Uses real coreutils (true/false) so it actually fork/execs.
// Skips gracefully if the binaries are not present.
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
int run(const std::string& src)
{
    auto state = std::make_unique<shell::ShellState>(std::map<std::string, std::string>{});
    auto disp  = std::make_unique<builtins::BuiltinDispatcher>();
    exec::Executor exec(state, disp);

    parser::Tokenizer tok(src);
    parser::Parser    p(tok.Tokenize());
    auto              tree = p.Parse();
    return exec.Run(*tree);
}
}

TEST(Executor, TrueReturnsZero)
{
    EXPECT_EQ(run("true"), 0);
}

TEST(Executor, FalseReturnsNonZero)
{
    EXPECT_NE(run("false"), 0);
}

TEST(Executor, AndShortCircuits)
{
    // false && true -> rhs skipped, status stays non-zero
    EXPECT_NE(run("false && true"), 0);
}

TEST(Executor, OrRunsRhsOnFailure)
{
    // false || true -> rhs runs, status 0
    EXPECT_EQ(run("false || true"), 0);
}

TEST(Executor, AndRunsRhsOnSuccess)
{
    // true && false -> rhs runs, status non-zero
    EXPECT_NE(run("true && false"), 0);
}

TEST(Executor, ListReturnsLastStatus)
{
    EXPECT_EQ(run("false ; true"), 0);
}

TEST(Executor, BangNegates)
{
    EXPECT_NE(run("! true"), 0);
    EXPECT_EQ(run("! false"), 0);
}

TEST(Executor, AssignmentOnlySucceeds)
{
    EXPECT_EQ(run("X=1"), 0);
}

TEST(Executor, UnknownCommandNonZero)
{
    EXPECT_NE(run("this_command_does_not_exist_xyz"), 0);
}
