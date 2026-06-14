#ifndef EXEC_WAIT_STATUS_HPP
#define EXEC_WAIT_STATUS_HPP

#include <cstdint>
#include <sys/types.h>

namespace exec
{
enum class WaitMode : std::uint8_t
{
    Poll,
    Foreground,
    UntilExit
};

class WaitStatus
{

public:

    explicit WaitStatus(pid_t pid, WaitMode mode);
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

    [[nodiscard]]
    bool IsStopped() const;

private:
    pid_t m_pid_;
    int m_status_;
    WaitMode m_waitMode_;
    bool m_running_;
}; // class WaitStatus
} // namespace exec
#endif