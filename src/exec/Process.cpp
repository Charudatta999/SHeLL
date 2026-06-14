#include "exec/Process.hpp"

#include "exec/ExecHelpers.hpp"
#include "exec/Redirection.hpp"
#include "io/FdOps.hpp"
#include "io/FileDescriptor.hpp"

#include <csignal>
#include <cstdio>
#include <unistd.h>
#include <vector>

namespace
{
const int stdinFD = 0;
const int stdOutFD = 1;
} // namespace

namespace exec
{
Process::Process() : m_pid_(-1) {};

pid_t Process::GetPid() const
{
    return m_pid_;
}

bool Process::Start(
    const CommandSpec& spec,
    const std::unique_ptr<io::FileDescriptor>& readFD,
    const std::unique_ptr<io::FileDescriptor>& writeFD,
    pid_t pgid)
{

    m_pid_ = fork();
    if (m_pid_ < 0)
    {
        return false;
    }

    if (m_pid_ == 0)
    {
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        setpgid(0, pgid);
        if (readFD != nullptr)
        {
            io::fdops::Dup2(*readFD, stdinFD);
        }
        if (writeFD != nullptr)
        {
            io::fdops::Dup2(*writeFD, stdOutFD);
        }
        for (const auto& redir : spec.redirects)
        {
            if (!exec::ApplyRedirect(redir))
                _exit(EXIT_FAILURE);
        }
        std::vector<char*> argv;

        argv.reserve(spec.argv.size() + 1);
        argv.push_back(const_cast<char*>(spec.argv[0].c_str()));
        for (size_t i = 1; i < spec.argv.size(); i++)
        {
            argv.push_back(const_cast<char*>(spec.argv[i].c_str()));
        }
        argv.push_back(nullptr);

        int retval = execvp(spec.argv[0].c_str(), argv.data());
        if (retval == -1)
        {
            _exit(EXIT_FAILURE);
        }
    }
    setpgid(m_pid_, pgid == 0 ? m_pid_ : pgid);
    return true;
}

} // namespace exec