#ifndef EXEC_PROCESS_HPP
#define EXEC_PROCESS_HPP
#include "io/FileDescriptor.hpp"
#include <fcntl.h>
#include <memory>
#include <string>
#include <vector>

namespace exec
{

class Process
{

public:
    Process();
    ~Process();

    // Non-Copyable and Non-Moveable
    Process(const Process& process) = delete;
    Process operator=(const Process& process) = delete;
    Process(Process&& process) = delete;
    Process& operator=(Process&& process) = delete;

    bool Start(const std::vector<std::string>& command,
         const std::unique_ptr<io::FileDescriptor>& readFD,
         const std::unique_ptr<io::FileDescriptor>& writeFD);

    [[nodiscard]]
    pid_t GetPid() const;
private:
    pid_t m_pid_;
};

} // namespace exec
#endif // EXEC_PROCESS_HPP