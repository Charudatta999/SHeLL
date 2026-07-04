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

TEST(ControlFlow, BreakExitsWhileLoopEarly)
{
    EXPECT_EQ(run("i=0; while ((i < 10)); do ((i++)); break; done; "
                  "((i == 1))"),
              0);
}

TEST(ControlFlow, BreakExitsForLoopOnFirstWord)
{
    EXPECT_EQ(run("for i in 1 2 3; do break; done; ((i == 1))"), 0);
}

TEST(ControlFlow, BreakStatusIsZero)
{
    EXPECT_EQ(run("for i in 1 2 3; do false; break; done"), 0);
}

TEST(ControlFlow, ContinueSkipsRestOfBody)
{
    EXPECT_EQ(run("n=0; for i in 1 2 3; do continue; ((n = n + 1)); "
                  "done; ((n == 0))"),
              0);
}

TEST(ControlFlow, ContinueStillRunsAllIterations)
{
    EXPECT_EQ(run("n=0; for i in 1 2 3; do ((n = n + 1)); continue; "
                  "done; ((n == 3))"),
              0);
}

TEST(ControlFlow, UntilLoopHonorsBreak)
{
    EXPECT_EQ(run("until false; do break; done"), 0);
}

TEST(ControlFlow, BreakInWhileConditionTerminates)
{
    EXPECT_EQ(run("while break; do ((0)); done"), 0);
}

TEST(ControlFlow, CStyleForContinueRunsUpdate)
{
    EXPECT_EQ(run("for ((i = 0; i < 3; i++)); do continue; done; "
                  "((i == 3))"),
              0);
}

TEST(ControlFlow, CStyleForBreakSkipsUpdate)
{
    EXPECT_EQ(run("for ((i = 0; i < 10; i++)); do break; done; "
                  "((i == 0))"),
              0);
}

TEST(ControlFlow, BreakTwoExitsOuterLoop)
{
    EXPECT_EQ(run("n=0; for i in 1 2; do ((n = n + 1)); "
                  "for j in 1 2; do break 2; done; ((n = n + 100)); "
                  "done; ((n == 1))"),
              0);
}

TEST(ControlFlow, ContinueTwoContinuesOuterLoop)
{
    EXPECT_EQ(run("n=0; for i in 1 2; do ((n = n + 1)); "
                  "for j in 1 2; do continue 2; done; "
                  "((n = n + 100)); done; ((n == 2))"),
              0);
}

TEST(ControlFlow, BreakLevelCapsAtOutermostLoop)
{
    EXPECT_EQ(run("for i in 1 2 3; do break 10; done"), 0);
}

TEST(ControlFlow, BreakInsideIfInsideLoop)
{
    EXPECT_EQ(run("for i in 1 2 3; do if ((i == 2)); then break; fi; "
                  "done; ((i == 2))"),
              0);
}

TEST(ControlFlow, BreakInsideGroupInsideLoop)
{
    EXPECT_EQ(run("for i in 1 2 3; do { break; }; done; ((i == 1))"),
              0);
}

TEST(ControlFlow, ReturnExitsFunctionWithStatus)
{
    EXPECT_EQ(run("f() { return 5; }; f"), 5);
}

TEST(ControlFlow, ReturnStopsFunctionBody)
{
    EXPECT_EQ(run("f() { return 3; return 4; }; f"), 3);
}

TEST(ControlFlow, ReturnUnwindsLoopInsideFunction)
{
    EXPECT_EQ(run("f() { for i in 1 2 3; do return 7; done; "
                  "return 1; }; f"),
              7);
}

TEST(ControlFlow, ReturnDoesNotEscapeNestedCall)
{
    EXPECT_EQ(run("f() { return 5; }; g() { f; return 6; }; g"), 6);
}

TEST(ControlFlow, FunctionBreakReachesCallersLoop)
{
    EXPECT_EQ(run("f() { break; }; n=0; for i in 1 2; do "
                  "((n = n + 1)); f; ((n = n + 100)); done; "
                  "((n == 1))"),
              0);
}

TEST(ControlFlow, BreakOutsideLoopWarnsAndSucceeds)
{
    EXPECT_EQ(run("break"), 0);
}

TEST(ControlFlow, ContinueOutsideLoopWarnsAndSucceeds)
{
    EXPECT_EQ(run("continue"), 0);
}

TEST(ControlFlow, ReturnOutsideFunctionFails)
{
    EXPECT_EQ(run("return"), 1);
    EXPECT_EQ(run("return 5"), 1);
}

TEST(ControlFlow, BreakZeroIsOutOfRange)
{
    EXPECT_EQ(run("for i in 1; do break 0; done"), 1);
}

TEST(ControlFlow, BreakNonNumericFails)
{
    EXPECT_EQ(run("for i in 1; do break foo; done"), 128);
}

TEST(ControlFlow, ReturnNonNumericFails)
{
    EXPECT_EQ(run("f() { return foo; }; f"), 255);
}
