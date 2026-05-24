#include "exec/WaitStatus.hpp"

#include <cerrno>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace exec
{

WaitStatus::WaitStatus(pid_t pid)
    : m_pid_(pid)
    , m_status_(-1)
{
    while (waitpid(m_pid_, &m_status_, 0) == -1)
    {
        if (errno != EINTR)
        {
            m_status_ = -1;
            break;
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

int WaitStatus::Signal() const
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

} // namespace exec