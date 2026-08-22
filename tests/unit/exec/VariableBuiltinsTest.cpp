// =========================================================
// VariableBuiltinsTest — export/unset/set/readonly through the
// full parse→exec pipeline, including the env a child observes.
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
    auto state = std::make_unique<shell::ShellState>(
        std::map<std::string, std::string>{});
    auto disp = std::make_unique<builtins::BuiltinDispatcher>();
    shell::expander::CommandRunner stubRunner =
        [](const std::string&) { return std::string{}; };
    exec::Executor exec(state, disp, stubRunner);

    parser::Tokenizer tok(src);
    parser::Parser p(tok.Tokenize());
    auto tree = p.Parse();
    return exec.Run(tree);
}
} // namespace

TEST(VariableBuiltins, ExportedVarReachesChildEnv)
{
    // Acceptance for #32: the child's environment contains it.
    EXPECT_EQ(
        run("export FOO=bar42 ; /usr/bin/env | grep -q FOO=bar42"),
        0);
}

TEST(VariableBuiltins, UnexportedVarDoesNotReachChild)
{
    EXPECT_NE(run("BAZ=xyz9 ; /usr/bin/env | grep -q BAZ=xyz9"), 0);
}

TEST(VariableBuiltins, UnsetRemovesVariable)
{
    EXPECT_EQ(run("X=hello ; unset X ; test x = x$X"), 0);
    EXPECT_NE(run("X=hello ; test x = x$X"), 0);
}

TEST(VariableBuiltins, SetDashDashAssignsPositionals)
{
    EXPECT_EQ(run("set -- a b c ; test b = $2"), 0);
    EXPECT_EQ(run("set -- a b c ; test 3 = $#"), 0);
}

TEST(VariableBuiltins, ReadonlyBlocksReassignment)
{
    EXPECT_NE(run("readonly X=1 ; X=2"), 0);
    EXPECT_NE(run("readonly X=1 ; unset X"), 0);
}

TEST(VariableBuiltins, ExportInvalidNameFails)
{
    EXPECT_NE(run("export 1BAD=x"), 0);
}

// #49: `set -m` is the user-facing job-control switch. The shell
// language exposes no way to read the flag back, so these drive the
// executor directly and query ShellState.
namespace
{
int runOn(std::unique_ptr<shell::ShellState>& state,
          const std::string& src)
{
    auto disp = std::make_unique<builtins::BuiltinDispatcher>();
    shell::expander::CommandRunner stubRunner =
        [](const std::string&) { return std::string{}; };
    exec::Executor exec(state, disp, stubRunner);

    parser::Tokenizer tok(src);
    parser::Parser p(tok.Tokenize());
    auto tree = p.Parse();
    return exec.Run(tree);
}

std::unique_ptr<shell::ShellState> freshState()
{
    return std::make_unique<shell::ShellState>(
        std::map<std::string, std::string>{});
}
} // namespace

TEST(VariableBuiltins, SetMonitorTogglesJobControl)
{
    auto state = freshState();
    ASSERT_FALSE(state->IsJobControlEnabled()); // default off

    EXPECT_EQ(runOn(state, "set -m"), 0);
    EXPECT_TRUE(state->IsJobControlEnabled());

    EXPECT_EQ(runOn(state, "set +m"), 0);
    EXPECT_FALSE(state->IsJobControlEnabled());
}

TEST(VariableBuiltins, SetMonitorLongFormMatchesShortForm)
{
    auto state = freshState();

    EXPECT_EQ(runOn(state, "set -o monitor"), 0);
    EXPECT_TRUE(state->IsJobControlEnabled());

    EXPECT_EQ(runOn(state, "set +o monitor"), 0);
    EXPECT_FALSE(state->IsJobControlEnabled());
}

TEST(VariableBuiltins, SetMonitorKeepsOptionMapInSync)
{
    auto state = freshState();

    EXPECT_EQ(runOn(state, "set -m"), 0);
    EXPECT_TRUE(state->IsOptionEnabled("monitor"));

    EXPECT_EQ(runOn(state, "set +m"), 0);
    EXPECT_FALSE(state->IsOptionEnabled("monitor"));
}

TEST(VariableBuiltins, SetMonitorCombinesWithOtherFlags)
{
    auto state = freshState();

    EXPECT_EQ(runOn(state, "set -em"), 0);
    EXPECT_TRUE(state->IsJobControlEnabled());
    EXPECT_TRUE(state->IsOptionEnabled("errexit"));

    EXPECT_EQ(runOn(state, "set +me"), 0);
    EXPECT_FALSE(state->IsJobControlEnabled());
    EXPECT_FALSE(state->IsOptionEnabled("errexit"));
}

TEST(VariableBuiltins, SetMonitorLeavesPositionalsAlone)
{
    auto state = freshState();
    EXPECT_EQ(runOn(state, "set -- a b c"), 0);
    EXPECT_EQ(runOn(state, "set -m"), 0);
    EXPECT_EQ(state->GetPositionalParams().size(), 3U);
}

TEST(VariableBuiltins, SetOptionStored)
{
    EXPECT_EQ(run("set -e"), 0);
    EXPECT_EQ(run("set -o pipefail"), 0);
    EXPECT_NE(run("set -q"), 0); // invalid option
}

TEST(VariableBuiltins, FunctionBindsPositionalParams)
{
    EXPECT_EQ(run("f() { test hello = $1; }; f hello"), 0);
    EXPECT_EQ(run("f() { test 2 = $#; }; f a b"), 0);
    // Caller positionals restored after the call.
    EXPECT_EQ(run("set -- outer; f() { return 0; }; f inner; "
                  "test outer = $1"),
              0);
}

TEST(VariableBuiltins, PrefixAssignmentOnBuiltin)
{
    // Prefix value is visible only for the builtin; restored after.
    EXPECT_EQ(run("FOO=orig; FOO=bar pwd >/dev/null; test orig = $FOO"),
              0);
    EXPECT_EQ(run("FOO=bar pwd >/dev/null; test x = x$FOO"), 0);
}

TEST(VariableBuiltins, PrefixAssignmentOnFunction)
{
    EXPECT_EQ(run("f() { test bar = $FOO; }; FOO=bar f"), 0);
    EXPECT_EQ(run("FOO=keep; f() { FOO=temp; }; FOO=bar f; "
                  "test keep = $FOO"),
              0);
    // Prefix assignments are exported for the callee's duration.
    EXPECT_EQ(
        run("f() { /usr/bin/env | grep -q '^FOO=bar$'; }; FOO=bar f"),
        0);
}

TEST(VariableBuiltins, ExportNameOnlyOmitsFromChildEnv)
{
    // POSIX/bash: export without assignment marks the name but does
    // not put NAME= into environ until assigned.
    EXPECT_NE(
        run("export EMPTY; /usr/bin/env | grep -q '^EMPTY='"),
        0);
}

TEST(VariableBuiltins, UnsetVarLeavesFunction)
{
    EXPECT_EQ(run("x=1; x() { return 0; }; unset x; test x = x$x"),
              0);
    // Bash: with no options, unset refers to the variable; the
    // function of the same name survives.
    EXPECT_EQ(run("x=1; x() { return 0; }; unset x; x"), 0);
}
