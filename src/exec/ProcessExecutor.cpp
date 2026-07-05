#include "exec/ProcessExecutor.hpp"

#include "exec/Redirection.hpp"
#include "io/FdOps.hpp"
#include "utils/ErrorCodes.hpp"


#include <cerrno>
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace exec
{
ProcessExecutor::ProcessExecutor()
    : m_pid_(-1)
    , m_running_(false)
    , m_exitCode_(-1)
{
}

pid_t ProcessExecutor::Pid() const
{
    return m_pid_;
}

bool ProcessExecutor::IsRunning() const
{
    return m_running_;
}

int ProcessExecutor::ExitCode() const
{
    return m_exitCode_;
}

void ProcessExecutor::Fork(const std::function<int()>& childFn, pid_t pgid)
{
    m_pid_ = fork();
    if (m_pid_ < 0)
    {
        m_running_ = false;
        return;
    }
    if (m_pid_ == 0)
    {
        setpgid(0, pgid);
        _exit(childFn());
    }
    setpgid(m_pid_, pgid == 0 ? m_pid_ : pgid);
    m_running_ = true;
}

int ProcessExecutor::Run(const std::function<int()>& childFn, pid_t pgid, WaitMode mode)
{
    Fork(childFn, pgid);
    if (m_pid_ < 0)
    {
        m_exitCode_ = -1;
        return m_exitCode_;
    }
    return Wait(mode);
}

void ProcessExecutor::Exec(const CommandSpec& spec)
{
    for (const auto& redir : spec.redirects)
    {
        if (!ApplyRedirect(redir))
            _exit(EXIT_FAILURE);
    }
    std::vector<char*> argv;
    argv.reserve(spec.argv.size() + 1);
    for (const auto& arg : spec.argv)
        argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);
    execvp(spec.argv[0].c_str(), argv.data());
    if(errno == ENOENT)
    {
        io::fdops::WriteAll(STDERR_FILENO, "command not found: " + spec.argv[0] + "\n");
        _exit(EXIT_COMMAND_NOT_FOUND);
    }
    else if (errno == EACCES)
    {
        io::fdops::WriteAll(STDERR_FILENO,
                            "permission denied: " + spec.argv[0] +
                                "\n");
        _exit(EXIT_PERMISSION_DENIED);
    }
    else
    {
        _exit(EXIT_FAILURE);
    }
}

int ProcessExecutor::Wait(WaitMode mode)
{
    WaitStatus wait(m_pid_, mode);
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
