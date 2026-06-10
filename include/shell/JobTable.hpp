#ifndef SHELL_JOBTABLE_HPP
#define SHELL_JOBTABLE_HPP
#include <string>
#include <sys/types.h>
#include <vector>

namespace shell
{

class JobTable
{
public:
    struct Job
    {
        int id;
        pid_t pid;
        std::string command;
        bool running;
    };

    JobTable();
    ~JobTable();
    JobTable(const JobTable&) = delete;
    JobTable& operator=(const JobTable&) = delete;
    JobTable(JobTable&&) = delete;
    JobTable& operator=(JobTable&&) = delete;

    std::vector<Job> Reap();

    int Add(pid_t pid, const std::string& command);

    [[nodiscard]]
    const std::vector<Job>& List() const;

private:
    std::vector<Job> m_jobs_;
    int m_nextId_ = 1;
};

} // namespace shell
#endif // SHELL_JOBTABLE_HPP