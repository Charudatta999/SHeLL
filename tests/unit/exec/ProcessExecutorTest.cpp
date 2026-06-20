// =========================================================
// ProcessExecutorTest — fork + run a callable in the child, wait, return code.
// Deterministic: the child runs the lambda and _exit()s, no exec.
// Exit codes are masked to 8 bits by the OS, so keep values in 0..255.
// =========================================================

#include <gtest/gtest.h>

#include "exec/ProcessExecutor.hpp"

using exec::ProcessExecutor;

TEST(ProcessExecutor, RunReturnsChildExitCode)
{
    ProcessExecutor proc;
    EXPECT_EQ(proc.Run([] { return 0; }, 0), 0);
}

TEST(ProcessExecutor, RunPropagatesNonZero)
{
    ProcessExecutor proc;
    EXPECT_EQ(proc.Run([] { return 7; }, 0), 7);
}

TEST(ProcessExecutor, RunMaxByte)
{
    ProcessExecutor proc;
    EXPECT_EQ(proc.Run([] { return 255; }, 0), 255);
}

TEST(ProcessExecutor, ChildActuallyRunsTheCallable)
{
    ProcessExecutor proc;
    int code = proc.Run([] {
        int count = 0;
        for (int idx = 0; idx < 5; ++idx)
            ++count;
        return count;
    }, 0);
    EXPECT_EQ(code, 5);
}

TEST(ProcessExecutor, PidPositiveAfterRun)
{
    ProcessExecutor proc;
    (void)proc.Run([] { return 0; }, 0);
    EXPECT_GT(proc.Pid(), 0);
}

TEST(ProcessExecutor, NotRunningAfterSyncRun)
{
    ProcessExecutor proc;
    (void)proc.Run([] { return 0; }, 0);
    EXPECT_FALSE(proc.IsRunning());
}

TEST(ProcessExecutor, ForkThenStateIsRunningPid)
{
    ProcessExecutor proc;
    proc.Fork([] { return 0; }, 0);
    EXPECT_GT(proc.Pid(), 0);
    // reap so we don't leak a zombie into the test runner
    (void)proc.IsRunning();
}
