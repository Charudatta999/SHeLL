#ifndef EXEC_FORKRUNNER_HPP
#define EXEC_FORKRUNNER_HPP

#include "exec/WaitStatus.hpp"

#include <functional>
#include <sys/types.h>

namespace exec
{

class ForkRunner
{
public:
    ForkRunner();
    ~ForkRunner() = default;
    ForkRunner(const ForkRunner&) = delete;
    ForkRunner& operator=(const ForkRunner&) = delete;
    ForkRunner(ForkRunner&&) = delete;
    ForkRunner& operator=(ForkRunner&&) = delete;

    [[nodiscard]]
    int Run(const std::function<int()>& childFn, WaitMode mode = WaitMode::Foreground);

    [[nodiscard]]
    pid_t Pid() const;

    [[nodiscard]]
    bool IsRunning() const;

    [[nodiscard]]
    int ExitCode() const;

    void Start(const std::function<int()>& childFn);

private:
    int Wait(WaitMode mode);
    pid_t m_pid_;
    bool m_running_;
    int m_exitCode_;
};
} // namespace exec
#endif // EXEC_FORKRUNNER_HPP