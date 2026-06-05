#ifndef EXEC_WAIT_STATUS_HPP
#define EXEC_WAIT_STATUS_HPP

#include <fcntl.h>

namespace exec
{

class WaitStatus
{

public:
    explicit WaitStatus(pid_t pid, bool background = false);
    ~WaitStatus() = default;

    // Non-Moveable & Non-Copyable
    WaitStatus(const WaitStatus& waitStatus) = delete;
    WaitStatus& operator=(const WaitStatus& waitStatus) = delete;

    WaitStatus(WaitStatus&& waitStatus) = delete;
    WaitStatus& operator=(WaitStatus&& waitStatus) = delete;

    [[nodiscard]]
    bool Exited() const;

    [[nodiscard]]
    int ExitCode() const;

    [[nodiscard]]
    bool Signaled() const;

    [[nodiscard]]
    int GetSignal() const;

    [[nodiscard]]
    bool IsValid() const;

    [[nodiscard]]
    bool IsRunning() const;

    [[nodiscard]]
    bool IsBackground() const;

private:
    pid_t m_pid_;
    int m_status_;
    bool m_background_;
    bool m_running_;
}; // class WaitStatus
} // namespace exec
#endif