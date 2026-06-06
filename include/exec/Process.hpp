#ifndef EXEC_PROCESS_HPP
#define EXEC_PROCESS_HPP
#include "io/FileDescriptor.hpp"
#include "exec/ExecHelpers.hpp"
#include <fcntl.h>
#include <memory>


namespace exec
{

class Process
{

public:
    Process();
    ~Process() = default;

    // Non-Copyable and Non-Moveable
    Process(const Process& process) = delete;
    Process operator=(const Process& process) = delete;
    Process(Process&& process) = delete;
    Process& operator=(Process&& process) = delete;

    bool Start(const CommandSpec& spec,
                    const std::unique_ptr<io::FileDescriptor>& readFD,
                    const std::unique_ptr<io::FileDescriptor>& writeFD,
                    pid_t pgid);

    [[nodiscard]]
    pid_t GetPid() const;
private:
    pid_t m_pid_;
};

} // namespace exec
#endif // EXEC_PROCESS_HPP