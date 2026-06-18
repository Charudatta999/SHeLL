#ifndef CORO_TASKPROMISE_HPP
#define CORO_TASKPROMISE_HPP

#include "coro/FinalAwaiter.hpp"

#include <coroutine>
#include <exception>
namespace coro
{
class Task;
class TaskPromise
{
public:
    TaskPromise();
    ~TaskPromise();
    TaskPromise(const TaskPromise&) = delete;
    TaskPromise& operator=(const TaskPromise&) = delete;
    TaskPromise(TaskPromise&&) = delete;
    TaskPromise& operator=(TaskPromise&&) = delete;

    Task get_return_object();

    std::suspend_always initial_suspend();

    FinalAwaiter final_suspend() noexcept;

    void return_value(int value);

    void unhandled_exception() noexcept;

    void SetContinuation(std::coroutine_handle<>);

    [[nodiscard]]

    std::coroutine_handle<> Continuation() const;

    [[nodiscard]]

    int Result() const;

    [[nodiscard]]

    std::exception_ptr Exception() const;

private:
    int m_result_;
    std::coroutine_handle<> m_continuation_;
    std::exception_ptr m_exception_;
};

} // namespace coro
#endif // CORO_TASKPROMISE_HPP