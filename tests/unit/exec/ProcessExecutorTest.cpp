// =========================================================
// ProcessExecutorTest — fork + run a callable in the child, wait, return code.
// Deterministic: the child runs the lambda and _exit()s, no exec.
// Exit codes are masked to 8 bits by the OS, so keep values in 0..255.
// =========================================================

#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "exec/ExecHelpers.hpp"
#include "exec/ProcessExecutor.hpp"
#include "utils/ErrorCodes.hpp"

using exec::CommandSpec;
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

// ─── exec failure exit codes (POSIX §2.8.2 contract) ─────────────────────────
// A command that isn't on PATH must exit 127 (not a silent generic failure).
// Regression for the bug where execvp failure just _exit(EXIT_FAILURE)'d.
TEST(ProcessExecutor, ExecMissingCommandExits127)
{
    ProcessExecutor proc;
    CommandSpec spec({"sh_no_such_command_zqxwv"});
    int code = proc.Run([&] { proc.Exec(spec); return 0; }, 0);
    EXPECT_EQ(code, EXIT_COMMAND_NOT_FOUND); // 127
}

// A path that exists but lacks the execute bit must exit 126.
TEST(ProcessExecutor, ExecNonExecutableExits126)
{
    const std::string path = "./process_executor_notexec.tmp";
    {
        std::ofstream file(path);
        file << "#!/bin/sh\necho hi\n";
    }
    ::chmod(path.c_str(), 0644); // readable but NOT executable

    ProcessExecutor proc;
    CommandSpec spec({path}); // has a '/', so execvp treats it as a path
    int code = proc.Run([&] { proc.Exec(spec); return 0; }, 0);

    ::unlink(path.c_str());
    EXPECT_EQ(code, EXIT_PERMISSION_DENIED); // 126
}
