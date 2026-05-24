#ifndef EXEC_PROCESS_HPP
#define EXEC_PROCESS_HPP
#include <fcntl.h>
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

    bool Start(const std::string& command, const std::vector<std::string>& args);

    [[nodiscard]]
    pid_t GetPid() const;
private:
    pid_t m_pid_;
};

} // namespace exec
#endif // EXEC_PROCESS_HPP