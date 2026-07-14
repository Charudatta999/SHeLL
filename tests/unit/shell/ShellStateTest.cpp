// =========================================================
// ShellStateTest — unit tests for shell::ShellState.
// Pure in-memory state: vars, export/env, exit codes, options,
// running flag, functions. No syscalls, fully deterministic.
// =========================================================

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>

#include "parser/ast/AstNode.hpp"
#include "parser/ast/commands/SimpleCommand.hpp"
#include "shell/ShellState.hpp"

using shell::ShellState;

namespace
{
ShellState make(const std::map<std::string, std::string>& vars = {})
{
    return ShellState(vars);
}
}

// ─── Construction / globals ──────────────────────────────────────────────────

TEST(ShellState, ConstructsWithGlobals)
{
    std::map<std::string, std::string> globals{{"FOO", "bar"}};
    ShellState s(globals);
    auto v = s.GetVar("FOO");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "bar");
}

TEST(ShellState, GetMissingVarReturnsNullopt)
{
    auto s = make();
    EXPECT_FALSE(s.GetVar("NOPE").has_value());
}

TEST(ShellState, CwdIsNonEmptyOnConstruction)
{
    auto s = make();
    EXPECT_FALSE(s.GetCWD().empty());
}

// ─── Variables ───────────────────────────────────────────────────────────────

TEST(ShellState, SetAndGetVar)
{
    auto s = make();
    s.SetVar("X", "1");
    auto v = s.GetVar("X");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "1");
}

TEST(ShellState, SetVarOverwrites)
{
    auto s = make();
    s.SetVar("X", "1");
    s.SetVar("X", "2");
    EXPECT_EQ(*s.GetVar("X"), "2");
}

TEST(ShellState, SetVarDefaultEmpty)
{
    auto s = make();
    s.SetVar("EMPTY");
    auto v = s.GetVar("EMPTY");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "");
}

TEST(ShellState, UnsetVarRemoves)
{
    auto s = make();
    s.SetVar("X", "1");
    s.UnSetVar("X");
    EXPECT_FALSE(s.GetVar("X").has_value());
}

TEST(ShellState, UnsetMissingVarIsNoop)
{
    auto s = make();
    s.UnSetVar("GHOST"); // must not crash
    SUCCEED();
}

// ─── CWD ─────────────────────────────────────────────────────────────────────

TEST(ShellState, SetCwd)
{
    auto s = make();
    s.SetCWD("/tmp/test");
    EXPECT_EQ(s.GetCWD(), "/tmp/test");
}

// ─── Export / environment ────────────────────────────────────────────────────

TEST(ShellState, NotExportedByDefault)
{
    auto s = make();
    s.SetVar("X", "1");
    EXPECT_FALSE(s.IsExported("X"));
}

TEST(ShellState, ExportVarMarksExported)
{
    auto s = make();
    s.SetVar("X", "1");
    s.ExportVar("X");
    EXPECT_TRUE(s.IsExported("X"));
}

TEST(ShellState, GetEnvOnlyContainsExported)
{
    auto s = make();
    s.SetVar("PUB", "1");
    s.SetVar("PRIV", "2");
    s.ExportVar("PUB");

    auto env = s.GetEnv();
    EXPECT_EQ(env.count("PUB"), 1u);
    EXPECT_EQ(env.count("PRIV"), 0u);
    EXPECT_EQ(env.at("PUB"), "1");
}

TEST(ShellState, GetLocalVarsExcludesExported)
{
    auto s = make();
    s.SetVar("PUB", "1");
    s.SetVar("PRIV", "2");
    s.ExportVar("PUB");

    auto local = s.GetLocalVars();
    EXPECT_EQ(local.count("PRIV"), 1u);
    EXPECT_EQ(local.count("PUB"), 0u);
}

TEST(ShellState, UnsetRemovesFromExport)
{
    auto s = make();
    s.SetVar("X", "1");
    s.ExportVar("X");
    s.UnSetVar("X");
    EXPECT_FALSE(s.IsExported("X"));
}

TEST(ShellState, ExportThenUpdateReflectsInEnv)
{
    auto s = make();
    s.SetVar("X", "old");
    s.ExportVar("X");
    s.SetVar("X", "new");          // single store; env is derived
    EXPECT_EQ(s.GetEnv().at("X"), "new");
}

// ─── Exit codes ──────────────────────────────────────────────────────────────

TEST(ShellState, LastExitCodeDefaultsZero)
{
    auto s = make();
    EXPECT_EQ(s.GetLastCommandExitCode(), 0);
}

TEST(ShellState, SetLastExitCode)
{
    auto s = make();
    s.SetLastCommandExitCode(42);
    EXPECT_EQ(s.GetLastCommandExitCode(), 42);
}

// ─── Options ─────────────────────────────────────────────────────────────────

TEST(ShellState, OptionDisabledByDefault)
{
    auto s = make();
    EXPECT_FALSE(s.IsOptionEnabled("pipefail"));
}

TEST(ShellState, SetOptionEnables)
{
    auto s = make();
    s.SetOption("pipefail");
    EXPECT_TRUE(s.IsOptionEnabled("pipefail"));
}

TEST(ShellState, DisableOption)
{
    auto s = make();
    s.SetOption("pipefail");
    s.DisableOption("pipefail");
    EXPECT_FALSE(s.IsOptionEnabled("pipefail"));
}

// ─── Running state ───────────────────────────────────────────────────────────

TEST(ShellState, RunningByDefault)
{
    auto s = make();
    EXPECT_TRUE(s.IsRunning());
}

TEST(ShellState, RequestExitStopsRunningAndSetsCode)
{
    auto s = make();
    s.RequestExit(7);
    EXPECT_FALSE(s.IsRunning());
    EXPECT_EQ(s.GetShellExitCode(), 7);
}

// ─── Functions ───────────────────────────────────────────────────────────────

TEST(ShellState, FunctionAbsentByDefault)
{
    auto s = make();
    EXPECT_FALSE(s.IsFunctionPresent("greet"));
    EXPECT_EQ(s.GetFunctionBody("greet"), nullptr);
}

TEST(ShellState, AddAndRetrieveFunction)
{
    auto s = make();
    auto body = std::make_unique<parser::ast::SimpleCommand>(
        std::vector<std::string>{"echo", "hi"},
        std::vector<parser::ast::Redirect>{},
        std::vector<std::pair<std::string, std::string>>{});
    parser::ast::AstNode* raw = body.get();

    s.AddFunction("greet", std::move(body));

    EXPECT_TRUE(s.IsFunctionPresent("greet"));
    EXPECT_EQ(s.GetFunctionBody("greet"), raw); // same node, owned by state
}

TEST(ShellState, UnsetFunctionRemoves)
{
    auto s = make();
    s.AddFunction("greet",
                  std::make_unique<parser::ast::SimpleCommand>(
                      std::vector<std::string>{"echo"},
                      std::vector<parser::ast::Redirect>{},
                      std::vector<std::pair<std::string, std::string>>{}));
    s.UnsetFunction("greet");
    EXPECT_FALSE(s.IsFunctionPresent("greet"));
}

TEST(ShellState, PidIsPositive)
{
    auto s = make();
    EXPECT_GT(s.GetShellPid(), 0);
}

// ─── Readonly ────────────────────────────────────────────────────────────────

TEST(ShellState, ReadonlyBlocksSetVar)
{
    auto s = make();
    s.SetVar("X", "1");
    s.MarkReadonly("X");
    EXPECT_FALSE(s.SetVar("X", "2"));
    EXPECT_EQ(*s.GetVar("X"), "1");
}

TEST(ShellState, ReadonlyBlocksUnset)
{
    auto s = make();
    s.SetVar("X", "1");
    s.MarkReadonly("X");
    EXPECT_FALSE(s.UnSetVar("X"));
    EXPECT_TRUE(s.GetVar("X").has_value());
}

TEST(ShellState, SetVarSucceedsWhenNotReadonly)
{
    auto s = make();
    EXPECT_TRUE(s.SetVar("X", "1"));
    EXPECT_TRUE(s.UnSetVar("X"));
}

TEST(ShellState, IsReadonlyReflectsMark)
{
    auto s = make();
    EXPECT_FALSE(s.IsReadonly("X"));
    s.MarkReadonly("X");
    EXPECT_TRUE(s.IsReadonly("X"));
    EXPECT_EQ(s.GetReadonlyVars().count("X"), 1u);
}

// ─── Constructor exports inherited vars ──────────────────────────────────────

TEST(ShellState, InheritedVarsAreExported)
{
    std::map<std::string, std::string> globals{{"PATH", "/bin"}};
    ShellState s(globals);
    EXPECT_TRUE(s.IsExported("PATH"));
    EXPECT_EQ(s.GetEnv().at("PATH"), "/bin");
}

// ─── Positional parameters ───────────────────────────────────────────────────

TEST(ShellState, PositionalParamsEmptyByDefault)
{
    auto s = make();
    EXPECT_TRUE(s.GetPositionalParams().empty());
}

TEST(ShellState, SetPositionalParamsReplaces)
{
    auto s = make();
    s.SetPositionalParams({"a", "b"});
    s.SetPositionalParams({"c"});
    ASSERT_EQ(s.GetPositionalParams().size(), 1u);
    EXPECT_EQ(s.GetPositionalParams()[0], "c");
}
