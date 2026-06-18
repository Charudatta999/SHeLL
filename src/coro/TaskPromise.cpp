#include "coro/TaskPromise.hpp"

#include "coro/FinalAwaiter.hpp"
#include "coro/Task.hpp"

#include <coroutine>
#include <exception>

namespace coro
{
TaskPromise::TaskPromise()
    : m_result_(0)
    , m_continuation_(nullptr)
    , m_exception_(nullptr)
{
}

TaskPromise::~TaskPromise() = default;

Task TaskPromise::get_return_object()
{
    return Task{
        std::coroutine_handle<TaskPromise>::from_promise(*this)};
}

std::suspend_always TaskPromise::initial_suspend()
{
    return {};
}

FinalAwaiter TaskPromise::final_suspend() noexcept
{
    return {};
}

void TaskPromise::return_value(int value)
{
    m_result_ = value;
}

void TaskPromise::unhandled_exception() noexcept
{
    m_exception_ = std::current_exception();
}

void TaskPromise::SetContinuation(std::coroutine_handle<> conti)
{
    m_continuation_ = conti;
}

std::coroutine_handle<> TaskPromise::Continuation() const
{
    return m_continuation_;
}

int TaskPromise::Result() const
{
    return m_result_;
}

std::exception_ptr TaskPromise::Exception() const
{
    return m_exception_;
}
} // namespace coro