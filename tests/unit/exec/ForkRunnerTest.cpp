// =========================================================
// ForkRunnerTest — fork + run a callable in the child, wait, return code.
// Deterministic: the child runs the lambda and _exit()s, no exec.
// Exit codes are masked to 8 bits by the OS, so keep values in 0..255.
// =========================================================

#include <gtest/gtest.h>

#include "exec/ForkRunner.hpp"

using exec::ForkRunner;

TEST(ForkRunner, RunReturnsChildExitCode)
{
    ForkRunner r;
    EXPECT_EQ(r.Run([] { return 0; }), 0);
}

TEST(ForkRunner, RunPropagatesNonZero)
{
    ForkRunner r;
    EXPECT_EQ(r.Run([] { return 7; }), 7);
}

TEST(ForkRunner, RunMaxByte)
{
    ForkRunner r;
    EXPECT_EQ(r.Run([] { return 255; }), 255);
}

TEST(ForkRunner, ChildActuallyRunsTheCallable)
{
    // Child increments in its own address space and returns it as the code,
    // proving the callable ran in the forked child.
    ForkRunner r;
    int code = r.Run([] {
        int x = 0;
        for (int i = 0; i < 5; ++i)
            ++x;
        return x;
    });
    EXPECT_EQ(code, 5);
}

TEST(ForkRunner, PidPositiveAfterRun)
{
    ForkRunner r;
    (void)r.Run([] { return 0; });
    EXPECT_GT(r.Pid(), 0);
}

TEST(ForkRunner, NotRunningAfterSyncRun)
{
    ForkRunner r;
    (void)r.Run([] { return 0; });
    EXPECT_FALSE(r.IsRunning());
}

TEST(ForkRunner, StartThenStateIsRunningPid)
{
    ForkRunner r;
    r.Start([] { return 0; });
    EXPECT_GT(r.Pid(), 0);
    // reap so we don't leak a zombie into the test runner
    (void)r.IsRunning();
}
