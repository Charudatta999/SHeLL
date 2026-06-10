#include "shell/JobTable.hpp"

#include "exec/WaitStatus.hpp"

namespace shell
{
JobTable::JobTable() = default;

JobTable::~JobTable() = default;

std::vector<JobTable::Job> JobTable::Reap()
{
    std::vector<Job> finished;
    for (auto itr = m_jobs_.begin(); itr != m_jobs_.end();)
    {
        auto waitStatus = exec::WaitStatus(itr->pid, true);
        if (!waitStatus.IsRunning())
        {
            finished.push_back(*itr);
            itr = m_jobs_.erase(itr);
        }
        else
        {
            ++itr;
        }
    }
    return finished;
}

int JobTable::Add(pid_t pid, const std::string& command)
{
    int jobId = m_nextId_++;
    m_jobs_.push_back({.id = jobId,
                       .pid = pid,
                       .command = command,
                       .running = true});
    return jobId;
}

const std::vector<JobTable::Job>& JobTable::List() const
{
    return m_jobs_;
}
} // namespace shell