#ifndef CORO_TASK_HPP
#define CORO_TASK_HPP

#include "coro/TaskPromise.hpp"

namespace coro
{

class Task
{
public:
    using promise_type = TaskPromise;
    explicit Task(std::coroutine_handle<promise_type> handle);
    ~Task();
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&&) noexcept;
    Task& operator=(Task&&) noexcept;

    bool await_ready();
    std::coroutine_handle<promise_type>
    await_suspend(std::coroutine_handle<> awaiter);
    int await_resume();
    [[nodiscard]]
    std::coroutine_handle<promise_type> GetHandle();

private:
    std::coroutine_handle<promise_type> m_handle_;
};

} // namespace coro
#endif // CORO_TASK_HPP