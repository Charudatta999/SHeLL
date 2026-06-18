#include "coro/Task.hpp"

#include "coro/FinalAwaiter.hpp"
#include "coro/TaskPromise.hpp"

#include <utility>

namespace coro
{
Task::Task(std::coroutine_handle<promise_type> handle)
    : m_handle_(handle)
{
}

Task::Task(Task&& other) noexcept
    : m_handle_(std::exchange(other.m_handle_, {}))
{
}

Task& Task::operator=(Task&& other) noexcept
{
    if (this != &other)
    {
        if (m_handle_)
            m_handle_.destroy();
        m_handle_ = std::exchange(other.m_handle_, {});
    }
    return *this;
}

Task::~Task()
{
    if (m_handle_)
    {
        m_handle_.destroy();
    }
}

std::coroutine_handle<Task::promise_type> Task::GetHandle()
{
    return m_handle_;
}

int Task::await_resume()
{
    if (auto excp = m_handle_.promise().Exception())
        std::rethrow_exception(excp);
    return m_handle_.promise().Result();
}

bool Task::await_ready()
{
    return false;
}

std::coroutine_handle<Task::promise_type>
Task::await_suspend(std::coroutine_handle<> awaiter)
{
    m_handle_.promise().SetContinuation(awaiter);
    return m_handle_;
}

} // namespace coro