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
#include "shell/ShellException.hpp"

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
    EXPECT_EQ(table.List()[0].state, shell::JobTable::State::Running);

    // cleanup: kill and reap directly so no zombie leaks
    ::kill(pid, SIGKILL);
    ::waitpid(pid, nullptr, 0);
}

TEST(JobTable, ReapMarksStoppedJobWithoutRemoving)
{
    shell::JobTable table;
    pid_t pid = spawnSleeping();
    table.Add(pid, "sleeper");

    ::kill(pid, SIGSTOP);
    // Poll until the stop is observed (signal delivery is async).
    for (int attempt = 0; attempt < 500; ++attempt)
    {
        table.Reap();
        if (table.List()[0].state == shell::JobTable::State::Stopped)
            break;
        ::usleep(1000);
    }

    ASSERT_EQ(table.List().size(), 1u); // stopped, NOT reaped away
    EXPECT_EQ(table.List()[0].state, shell::JobTable::State::Stopped);
    EXPECT_TRUE(table.Reap().empty()); // still not collectable

    // cleanup: resume, kill, reap directly so no zombie leaks
    ::kill(pid, SIGCONT);
    ::kill(pid, SIGKILL);
    ::waitpid(pid, nullptr, 0);
}

TEST(JobTable, EmptyTableReapIsEmpty)
{
    shell::JobTable table;
    EXPECT_TRUE(table.Reap().empty());
    EXPECT_TRUE(table.List().empty());
}

// ─── FindById / UpdateJobState / RemoveByID (no fork needed) ──────────────────
TEST(JobTable, FindByIdReturnsMatchingJob)
{
    shell::JobTable table;
    table.Add(1111, "a");
    int id = table.Add(2222, "b");

    auto& job = table.FindById(id);
    EXPECT_EQ(job.id, id);
    EXPECT_EQ(job.pid, 2222);
    EXPECT_EQ(job.command, "b");
}

TEST(JobTable, FindByIdThrowsWhenMissing)
{
    shell::JobTable table;
    table.Add(1111, "a");
    EXPECT_THROW((void)table.FindById(99), shell::ShellException);
}

TEST(JobTable, UpdateJobStateChangesStateById)
{
    shell::JobTable table;
    int id = table.Add(1234, "sleeper"); // defaults to Running
    ASSERT_EQ(table.List()[0].state, shell::JobTable::State::Running);

    table.UpdateJobState(id, shell::JobTable::State::Stopped);
    EXPECT_EQ(table.FindById(id).state, shell::JobTable::State::Stopped);

    table.UpdateJobState(id, shell::JobTable::State::Running);
    EXPECT_EQ(table.FindById(id).state, shell::JobTable::State::Running);
}

TEST(JobTable, RemoveByIdErasesOnlyThatJob)
{
    shell::JobTable table;
    int first  = table.Add(1111, "a");
    int second = table.Add(2222, "b");

    table.RemoveByID(first);

    ASSERT_EQ(table.List().size(), 1u);
    EXPECT_EQ(table.List()[0].id, second);
    EXPECT_THROW((void)table.FindById(first), shell::ShellException);
}

TEST(JobTable, RemoveByIdMissingIsNoop)
{
    shell::JobTable table;
    table.Add(1111, "a");
    table.RemoveByID(99); // not present
    EXPECT_EQ(table.List().size(), 1u);
}

// ─── current-job recency (#42) ───────────────────────────────────────────────
TEST(JobTable, CurrentIdEmptyWhenNoJobs)
{
    shell::JobTable table;
    EXPECT_FALSE(table.CurrentId().has_value());
}

TEST(JobTable, NewestAddedJobIsCurrent)
{
    shell::JobTable table;
    table.Add(1111, "a");
    int second = table.Add(2222, "b");
    EXPECT_EQ(table.CurrentId(), second);
}

// The bug this feature fixes: the current job follows *use*, not start order.
// Stop an OLDER job while a newer one exists -> the older becomes current.
TEST(JobTable, StoppingOlderJobMakesItCurrent)
{
    shell::JobTable table;
    int older = table.Add(1111, "a");
    table.Add(2222, "b"); // "b" is current (added last)

    table.UpdateJobState(older, shell::JobTable::State::Stopped);

    EXPECT_EQ(table.CurrentId(), older); // not "b"
}

// When the current job ends, the previous job becomes current.
TEST(JobTable, EndingCurrentJobPromotesPrevious)
{
    shell::JobTable table;
    int first  = table.Add(1111, "a");
    int second = table.Add(2222, "b"); // current
    ASSERT_EQ(table.CurrentId(), second);

    table.RemoveByID(second);
    EXPECT_EQ(table.CurrentId(), first);
}

// Reaping a finished current job must drop it from recency (real fork; this is
// the path with the erase-before-vs-after-iterator bug).
TEST(JobTable, ReapedDoneJobStopsBeingCurrent)
{
    shell::JobTable table;
    pid_t sleeper = spawnSleeping();
    int   keep    = table.Add(sleeper, "sleeper"); // older, stays running
    pid_t exiting = spawnExiting(0);
    table.Add(exiting, "true"); // newest -> current

    reapUntilDone(table); // collect the exited job

    ASSERT_TRUE(table.CurrentId().has_value());
    EXPECT_EQ(table.CurrentId(), keep); // survivor took over; dead job gone

    ::kill(sleeper, SIGKILL);
    ::waitpid(sleeper, nullptr, 0);
}
