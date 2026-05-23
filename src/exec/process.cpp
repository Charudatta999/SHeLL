#include "exec/Process.hpp"

#include <string>
#include <unistd.h>

namespace exec
{
Process::Process()
    : m_pid_(-1) {};

pid_t Process::GetPid() const
{
    return m_pid_;
}

bool Process::Start(const std::string& command, const std::vector<std::string>& args)
{
    m_pid_ = fork();
    if (m_pid_ < 0)
    {
        return false;
    }

    if (m_pid_ == 0)
    {
        std::vector<char*> argv;
        argv.reserve(args.size() + 2);
        argv.push_back(const_cast<char*>(command.c_str()));
        for (const auto& arg : args)
        {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        int retval = execvp(command.c_str(), argv.data());
        if( retval == -1)
        {
            _exit(EXIT_FAILURE);
        }
    }

    return true;
}

} // namespace exec