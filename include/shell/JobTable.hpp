#ifndef SHELL_JOBTABLE_HPP
#define SHELL_JOBTABLE_HPP
#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>

namespace shell
{

class JobTable
{
public:
    enum class State : std::uint8_t
    {
        Running,
        Stopped,
        Done
    };

    struct Job
    {
        int id;
        pid_t pid;
        std::string command;
        State state;
    };

    JobTable();
    ~JobTable();
    JobTable(const JobTable&) = delete;
    JobTable& operator=(const JobTable&) = delete;
    JobTable(JobTable&&) = delete;
    JobTable& operator=(JobTable&&) = delete;

    std::vector<Job> Reap();

    int Add(pid_t pid, const std::string& command, State state= State::Running);

    [[nodiscard]]
    const std::vector<Job>& List() const;
    void UpdateJobState(int id, State state);
    [[nodiscard]]
    Job& FindById(int id);

    void RemoveByID(int id);

private:
    std::vector<Job> m_jobs_;
    int m_nextId_ = 1;
};

} // namespace shell
#endif // SHELL_JOBTABLE_HPP