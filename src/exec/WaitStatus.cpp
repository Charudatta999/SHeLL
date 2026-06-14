#include "exec/WaitStatus.hpp"

#include <cerrno>
#include <sys/wait.h>
#include <unistd.h>

namespace exec
{

WaitStatus::WaitStatus(pid_t pid, WaitMode mode)
    : m_pid_(pid)
    , m_status_(-1)
    , m_waitMode_(mode)
    , m_running_(true)
{
    if (mode == WaitMode::Poll)
    {
        pid_t childPid{-1};
        while ((childPid = waitpid(m_pid_, &m_status_, WNOHANG | WUNTRACED)) == -1 && errno == EINTR)
        {
        }
        if (childPid == 0)
        {
            m_running_ = true;
        }
        else if (childPid == m_pid_)
        {
            m_running_ = false;
        }
        else
        {
            m_running_ = false;
            m_status_ = -1;
        }
    }
    else if (mode == WaitMode::Foreground)
    {
        while (waitpid(m_pid_, &m_status_, WUNTRACED ) == -1)
        {
            if (errno != EINTR)
            {
                m_status_ = -1;
                break;
            }
        }
        m_running_ = false;
    }
    else if(mode == WaitMode::UntilExit)
    {
        pid_t childPid{-1};
        while ((childPid = waitpid(m_pid_, &m_status_, 0)) == -1 && errno == EINTR)
        {
        }
        if (childPid == 0)
        {
            m_running_ = true;
        }
        else if (childPid == m_pid_)
        {
            m_running_ = false;
        }
        else
        {
            m_running_ = false;
            m_status_ = -1;
        }
    }
}

bool WaitStatus::Exited() const
{
    // WIFEXITED is a macro that takes the status value and returns non-zero
    // if the child terminated normally.
    return m_status_ != -1 && WIFEXITED(m_status_);
}

int WaitStatus::ExitCode() const
{
    if (!Exited())
    {
        return -1;
    }
    return WEXITSTATUS(m_status_);
}

bool WaitStatus::Signaled() const
{
    return m_status_ != -1 && WIFSIGNALED(m_status_);
}

int WaitStatus::GetSignal() const
{

    if (Signaled())
    {
        return WTERMSIG(m_status_);
    }
    return -1;
}

bool WaitStatus::IsValid() const
{
    return m_status_ != -1;
}

bool WaitStatus::IsRunning() const
{
    return m_running_;
}

bool WaitStatus::IsBackground() const
{
    return m_waitMode_ == WaitMode::Poll;
}

bool WaitStatus::IsStopped() const
{
    return IsValid() && WIFSTOPPED(m_status_);
}
} // namespace exec