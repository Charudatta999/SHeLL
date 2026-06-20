#ifndef EXEC_PROCESS_EXECUTOR_HPP
#define EXEC_PROCESS_EXECUTOR_HPP

#include "exec/ExecHelpers.hpp"
#include "exec/WaitStatus.hpp"

#include <functional>
#include <sys/types.h>

namespace exec
{

class ProcessExecutor
{
public:
    ProcessExecutor();
    ~ProcessExecutor() = default;
    ProcessExecutor(const ProcessExecutor&) = delete;
    ProcessExecutor& operator=(const ProcessExecutor&) = delete;
    ProcessExecutor(ProcessExecutor&&) = delete;
    ProcessExecutor& operator=(ProcessExecutor&&) = delete;

    [[nodiscard]]
    int Run(const std::function<int()>& childFn, pid_t pgid, WaitMode mode = WaitMode::Foreground);

    [[nodiscard]]
    pid_t Pid() const;

    [[nodiscard]]
    bool IsRunning() const;

    [[nodiscard]]
    int ExitCode() const;

    void Fork(const std::function<int()>& childFn, pid_t pgid);

    [[noreturn]]
    void Exec(const CommandSpec& spec);

private:
    int Wait(WaitMode mode);
    pid_t m_pid_;
    bool m_running_;
    int m_exitCode_;
};
} // namespace exec
#endif // EXEC_PROCESS_EXECUTOR_HPP
