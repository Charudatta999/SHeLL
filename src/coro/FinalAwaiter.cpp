#include "coro/FinalAwaiter.hpp"

#include "coro/TaskPromise.hpp"

namespace coro
{

bool FinalAwaiter::await_ready() noexcept
{
    return false;
}

std::coroutine_handle<> FinalAwaiter::await_suspend(
    std::coroutine_handle<TaskPromise> self) noexcept
{
    auto cont = self.promise().Continuation();
    return cont ? cont : std::noop_coroutine();
}

void FinalAwaiter::await_resume() noexcept {}
} // namespace coro