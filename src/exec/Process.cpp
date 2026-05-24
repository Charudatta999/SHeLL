#include "exec/Process.hpp"
#include "io/FdOps.hpp"
#include "io/FileDescriptor.hpp"
#include <cstdio>
#include <string>
#include <unistd.h>
namespace
{
 const int stdinFD = 0;
 const int stdOutFD = 1;
}
namespace exec
{
Process::Process()
    : m_pid_(-1) {};

pid_t Process::GetPid() const
{
    return m_pid_;
}

bool Process::Start(const std::vector<std::string>& command,
                const std::unique_ptr<io::FileDescriptor>& readFD,
                const std::unique_ptr<io::FileDescriptor>& writeFD)
{
    m_pid_ = fork();
    if (m_pid_ < 0)
    {
        return false;
    }

    if (m_pid_ == 0)
    {

        if(!readFD)
        {
            io::fdops::Dup2(*readFD, stdinFD);
        }
        if(!writeFD)
        {
            io::fdops::Dup2(*writeFD,stdOutFD);
        }
        std::vector<char*> argv;
        argv.reserve(command.size() + 1);
        argv.push_back(const_cast<char*>(command[0].c_str()));
        for (size_t i = 1; i < command.size(); i++)
        {
            argv.push_back(const_cast<char*>(command[i].c_str()));
        }
        argv.push_back(nullptr);

        int retval = execvp(command[0].c_str(), argv.data());
        if( retval == -1)
        {
            _exit(EXIT_FAILURE);
        }
    }
    return true;
}

} // namespace exec