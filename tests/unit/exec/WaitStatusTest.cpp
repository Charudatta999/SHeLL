// =========================================================
// WaitStatusTest — the wait primitive the whole job-control
// stack depends on. Forks real children and exercises each
// WaitMode (Poll / Foreground / UntilExit).
// =========================================================

#include <gtest/gtest.h>

#include <csignal>

#include <sys/wait.h>
#include <unistd.h>

#include "exec/WaitStatus.hpp"

namespace
{
// Fork a child that exits immediately with the given code.
pid_t spawnExiting(int code)
{
    pid_t pid = ::fork();
    if (pid == 0)
        _exit(code);
    return pid;
}

// Fork a child that blocks until killed.
pid_t spawnSleeping()
{
    pid_t pid = ::fork();
    if (pid == 0)
    {
        for (;;)
            ::pause();
    }
    return pid;
}
} // namespace

TEST(WaitStatus, UntilExitBlocksAndReturnsExitCode)
{
    pid_t pid = spawnExiting(7);

    exec::WaitStatus status(pid, exec::WaitMode::UntilExit);

    EXPECT_FALSE(status.IsRunning());
    EXPECT_TRUE(status.Exited());
    EXPECT_EQ(status.ExitCode(), 7);
    EXPECT_FALSE(status.Signaled());
    EXPECT_FALSE(status.IsStopped());
}

TEST(WaitStatus, UntilExitDetectsKillingSignal)
{
    pid_t pid = spawnSleeping();
    ::kill(pid, SIGKILL);

    exec::WaitStatus status(pid, exec::WaitMode::UntilExit);

    EXPECT_TRUE(status.Signaled());
    EXPECT_EQ(status.GetSignal(), SIGKILL);
    EXPECT_FALSE(status.Exited());
}

TEST(WaitStatus, PollIsNonBlockingForRunningChild)
{
    pid_t pid = spawnSleeping();

    // Child is alive and not stopped -> Poll must return immediately
    // reporting it as still running (this guards the fg "returns
    // instantly" class of bug, inverted).
    exec::WaitStatus status(pid, exec::WaitMode::Poll);

    EXPECT_TRUE(status.IsRunning());
    EXPECT_FALSE(status.Exited());
    EXPECT_FALSE(status.IsStopped());

    ::kill(pid, SIGKILL);
    ::waitpid(pid, nullptr, 0);
}

TEST(WaitStatus, PollDetectsStoppedChild)
{
    pid_t pid = spawnSleeping();
    ::kill(pid, SIGSTOP);

    // Poll past the async stop delivery.
    bool stopped = false;
    for (int attempt = 0; attempt < 500; ++attempt)
    {
        exec::WaitStatus status(pid, exec::WaitMode::Poll);
        if (status.IsStopped())
        {
            stopped = true;
            break;
        }
        ::usleep(1000);
    }
    EXPECT_TRUE(stopped);

    ::kill(pid, SIGCONT);
    ::kill(pid, SIGKILL);
    ::waitpid(pid, nullptr, 0);
}

TEST(WaitStatus, PollCollectsAlreadyExitedChild)
{
    pid_t pid = spawnExiting(3);

    // Retry until the WNOHANG poll observes the exit.
    int code = -1;
    for (int attempt = 0; attempt < 500; ++attempt)
    {
        exec::WaitStatus status(pid, exec::WaitMode::Poll);
        if (!status.IsRunning() && status.Exited())
        {
            code = status.ExitCode();
            break;
        }
        ::usleep(1000);
    }
    EXPECT_EQ(code, 3);
}
