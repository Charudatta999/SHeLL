// =========================================================
// JobTableTest — background job tracking + reaping.
// Forks real children (Reap uses waitpid), so these tests
// spawn short-lived / sleeping processes.
// =========================================================

#include <gtest/gtest.h>

#include <csignal>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "shell/JobTable.hpp"

namespace
{
// Fork a child that exits immediately with the given code.
pid_t spawnExiting(int code = 0)
{
    pid_t pid = ::fork();
    if (pid == 0)
        _exit(code);
    return pid;
}

// Fork a child that blocks until killed (so it stays "running").
pid_t spawnSleeping()
{
    pid_t pid = ::fork();
    if (pid == 0)
    {
        for (;;)
            ::pause(); // wait for a signal forever
    }
    return pid;
}

// Poll Reap() until it returns the finished job(s) or we give up.
std::vector<shell::JobTable::Job> reapUntilDone(shell::JobTable& table)
{
    std::vector<shell::JobTable::Job> finished;
    for (int attempt = 0; attempt < 500 && finished.empty(); ++attempt)
    {
        finished = table.Reap();
        if (finished.empty())
            ::usleep(1000); // 1ms; child exit is async
    }
    return finished;
}
} // namespace

TEST(JobTable, AddAssignsIncrementingIds)
{
    shell::JobTable table;
    pid_t first  = spawnExiting();
    pid_t second = spawnExiting();

    EXPECT_EQ(table.Add(first, "a"), 1);
    EXPECT_EQ(table.Add(second, "b"), 2);
    EXPECT_EQ(table.List().size(), 2u);

    reapUntilDone(table); // clean up zombies
    table.Reap();
}

TEST(JobTable, ReapCollectsFinishedJob)
{
    shell::JobTable table;
    pid_t pid = spawnExiting(0);
    int   id  = table.Add(pid, "true");

    auto finished = reapUntilDone(table);

    ASSERT_EQ(finished.size(), 1u);
    EXPECT_EQ(finished[0].id, id);
    EXPECT_EQ(finished[0].pid, pid);
    EXPECT_EQ(finished[0].command, "true");
    EXPECT_TRUE(table.List().empty()); // removed after reaping
}

TEST(JobTable, ReapLeavesRunningJob)
{
    shell::JobTable table;
    pid_t pid = spawnSleeping();
    table.Add(pid, "sleeper");

    auto finished = table.Reap(); // immediate poll — still running
    EXPECT_TRUE(finished.empty());
    ASSERT_EQ(table.List().size(), 1u);
    EXPECT_TRUE(table.List()[0].running);

    // cleanup: kill and reap directly so no zombie leaks
    ::kill(pid, SIGKILL);
    ::waitpid(pid, nullptr, 0);
}

TEST(JobTable, EmptyTableReapIsEmpty)
{
    shell::JobTable table;
    EXPECT_TRUE(table.Reap().empty());
    EXPECT_TRUE(table.List().empty());
}
