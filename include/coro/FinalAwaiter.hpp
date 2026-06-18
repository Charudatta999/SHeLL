#ifndef CORO_FINALAWAITER_HPP
#define CORO_FINALAWAITER_HPP

#include <coroutine>


namespace coro
{
class TaskPromise;

class FinalAwaiter
{
public:

    FinalAwaiter() = default;
    ~FinalAwaiter() = default;
    FinalAwaiter(const FinalAwaiter&) = default;
    FinalAwaiter& operator=(const FinalAwaiter&) = default;
    FinalAwaiter(FinalAwaiter&&) = default;
    FinalAwaiter& operator=(FinalAwaiter&&) = default;

    bool await_ready() noexcept;
    std::coroutine_handle<>
    await_suspend(std::coroutine_handle<TaskPromise> self) noexcept;
    void await_resume() noexcept;
};

} // namespace coro
#endif // CORO_FINALAWAITER_HPP