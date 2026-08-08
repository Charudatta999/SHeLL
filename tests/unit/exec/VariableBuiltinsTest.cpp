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
