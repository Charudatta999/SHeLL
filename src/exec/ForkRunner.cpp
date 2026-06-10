#include "exec/ForkRunner.hpp"

#include "exec/WaitStatus.hpp"
#include "utils/ErrorCodes.hpp"
#include <sys/types.h>
#include <unistd.h>

namespace
{
constexpr int SIGNAL_EXIT_BASE = 128;
}

namespace exec
{
ForkRunner::ForkRunner()
    : m_pid_(-1)
    , m_running_(false)
    , m_exitCode_(-1)
{
}

pid_t ForkRunner::Pid() const
{
    return m_pid_;
}

bool ForkRunner::IsRunning() const
{
    return m_running_;
}

int ForkRunner::ExitCode() const
{
    return m_exitCode_;
}

void ForkRunner::Start(const std::function<int()>& childFn)
{
    m_pid_ = fork();
    if (m_pid_ < 0)
    {
        m_running_ = false;
        return;
    }
    if (m_pid_ == 0)
        _exit(childFn());
    m_running_ = true;
}

int ForkRunner::Run(const std::function<int()>& childFn, bool background)
{
    Start(childFn);
    if (m_pid_ < 0)
    {
        m_exitCode_ = -1;
        return m_exitCode_;
    }
    return Wait(background);
}

int ForkRunner::Wait(bool background)
{

    WaitStatus wait(m_pid_, background);
    if (wait.IsRunning())
    {
        m_running_ = true;
        return PROCESS_RUNNING;
    }
    if (wait.Exited())
    {
        m_exitCode_ = wait.ExitCode();
    }
    else if (wait.Signaled())
    {
        m_exitCode_ = SIGNAL_EXIT_BASE + wait.GetSignal();
    }
    m_running_ = false;
    return m_exitCode_;
}

} // namespace exec