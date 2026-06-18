#ifndef EXEC_SUSPENDED_CORO_HPP
#define EXEC_SUSPENDED_CORO_HPP

#include "coro/Task.hpp"

#include <coroutine>
#include <functional>
#include <memory>
#include <sys/types.h>

namespace parser::ast
{
class AstNode;
}

namespace exec
{

struct SuspendedCoro
{
    struct ResumeResult
    {
        bool completed;
        int status;
        pid_t newPgid;
    };

    coro::Task rootTask;
    std::coroutine_handle<> leafHandle;
    std::shared_ptr<parser::ast::AstNode> ast;
    std::function<ResumeResult(int leafStatus)> resume;
};

} // namespace exec
#endif // EXEC_SUSPENDED_CORO_HPP