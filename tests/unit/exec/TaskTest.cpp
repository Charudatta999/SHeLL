// =========================================================
// TaskTest — the coroutine primitive the resumable executor
// is built on. Verifies lazy start, value return, and nested
// awaiting (symmetric transfer).
// =========================================================

#include <gtest/gtest.h>

#include "coro/Task.hpp"

namespace
{
coro::Task makeValue(int value)
{
    co_return value;
}

coro::Task addOne(int value)
{
    int inner = co_await makeValue(value);
    co_return inner + 1;
}

coro::Task sumTwo(int a, int b)
{
    int first = co_await makeValue(a);
    int second = co_await makeValue(b);
    co_return first + second;
}

// Drive a lazy task to completion and read its result.
int run(coro::Task& task)
{
    task.GetHandle().resume();
    return task.GetHandle().promise().Result();
}
} // namespace

TEST(Task, LazyUntilResumed)
{
    auto task = makeValue(42);
    // Created suspended at initial_suspend — not yet done.
    EXPECT_FALSE(task.GetHandle().done());

    EXPECT_EQ(run(task), 42);
    EXPECT_TRUE(task.GetHandle().done());
}

TEST(Task, AwaitNestedTask)
{
    auto task = addOne(5);
    EXPECT_EQ(run(task), 6); // co_await makeValue(5) -> 5, +1
}

TEST(Task, AwaitMultipleSequential)
{
    auto task = sumTwo(3, 4);
    EXPECT_EQ(run(task), 7);
}
