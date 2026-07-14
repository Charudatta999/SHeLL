// =========================================================
// CStyleForTest — execution semantics of the C-style for loop
// (PR #26): init runs once, cond gates each iteration (empty
// cond is infinite), update runs after the body, and the loop
// variable persists after the loop. Loop-control interplay
// (break/continue) lives in ControlFlowTest.
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
    parser::Parser    p(tok.Tokenize());
    auto              tree = p.Parse();
    return exec.Run(tree);
}
} // namespace

TEST(CStyleFor, RunsExpectedIterationCount)
{
    EXPECT_EQ(run("n=0; for ((i = 0; i < 5; i++)); do ((n = n + 1)); "
                  "done; ((n == 5))"),
              0);
}

TEST(CStyleFor, BodySeesLoopVariableEachIteration)
{
    // 1 + 2 + 3 + 4
    EXPECT_EQ(run("s=0; for ((i = 1; i <= 4; i++)); do "
                  "((s = s + i)); done; ((s == 10))"),
              0);
}

TEST(CStyleFor, LoopVariablePersistsAfterLoop)
{
    EXPECT_EQ(run("for ((i = 0; i < 4; i++)); do ((1)); done; "
                  "((i == 4))"),
              0);
}

TEST(CStyleFor, ZeroIterationsWhenConditionInitiallyFalse)
{
    EXPECT_EQ(run("n=0; for ((i = 0; i < 0; i++)); do "
                  "((n = n + 1)); done; ((n == 0))"),
              0);
}

TEST(CStyleFor, InitOverwritesExistingVariable)
{
    EXPECT_EQ(run("i=99; n=0; for ((i = 0; i < 2; i++)); do "
                  "((n = n + 1)); done; ((n == 2))"),
              0);
}

TEST(CStyleFor, EmptyInitUsesExistingVariable)
{
    EXPECT_EQ(run("i=2; n=0; for ((; i < 5; i++)); do "
                  "((n = n + 1)); done; ((n == 3))"),
              0);
}

TEST(CStyleFor, EmptyConditionLoopsUntilBreak)
{
    EXPECT_EQ(run("n=0; for ((i = 0;; i++)); do "
                  "if ((i == 3)); then break; fi; ((n = n + 1)); "
                  "done; ((n == 3))"),
              0);
}

TEST(CStyleFor, EmptyUpdateAdvancesInBody)
{
    EXPECT_EQ(run("n=0; for ((i = 0; i < 3;)); do ((i = i + 1)); "
                  "((n = n + 1)); done; ((n == 3))"),
              0);
}

TEST(CStyleFor, AllEmptyHeaderLoopsUntilBreak)
{
    EXPECT_EQ(run("n=0; for ((;;)); do ((n = n + 1)); "
                  "if ((n == 4)); then break; fi; done; ((n == 4))"),
              0);
}

TEST(CStyleFor, NestedLoopsMultiply)
{
    EXPECT_EQ(run("t=0; for ((i = 0; i < 3; i++)); do "
                  "for ((j = 0; j < 3; j++)); do ((t = t + 1)); "
                  "done; done; ((t == 9))"),
              0);
}

TEST(CStyleFor, ParenthesizedSectionExpressions)
{
    EXPECT_EQ(run("n=0; for ((i = (2 * 3); i > (0); i = (i - 1))); "
                  "do ((n = n + 1)); done; ((n == 6))"),
              0);
}

TEST(CStyleFor, BodyRunsShellCommandsNotJustArithmetic)
{
    EXPECT_EQ(run("n=0; for ((i = 0; i < 2; i++)); do "
                  "m=5; ((n = n + m)); done; ((n == 10))"),
              0);
}
