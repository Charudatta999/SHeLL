#include "shell/JobTable.hpp"

#include "exec/WaitStatus.hpp"
#include "shell/ShellException.hpp"

namespace shell
{
JobTable::JobTable() = default;

JobTable::~JobTable() = default;

std::vector<JobTable::Job> JobTable::Reap()
{
    std::vector<Job> jobEvents;
    for (auto itr = m_jobs_.begin(); itr != m_jobs_.end();)
    {
        exec::WaitStatus waitStatus(itr->pid, exec::WaitMode::Poll);
        if (!waitStatus.IsRunning())
        {
            if(waitStatus.IsStopped())
            {
                itr->state = State::Stopped;
                jobEvents.push_back(*itr);
                ++itr;
                continue;
            }
            itr->state = State::Done;
            jobEvents.push_back(*itr);
            itr = m_jobs_.erase(itr);
        }
        else
        {
            ++itr;
        }
    }
    return jobEvents;
}

int JobTable::Add(pid_t pid, const std::string& command, State state)
{
    int jobId = m_nextId_++;
    m_jobs_.push_back({.id = jobId,
                       .pid = pid,
                       .command = command,
                       .state = state});
    return jobId;
}

const std::vector<JobTable::Job>& JobTable::List() const
{
    return m_jobs_;
}

void JobTable::UpdateJobState(int id, State state)
{
    for(auto& job : m_jobs_)
    {
        if(job.id == id)
        {
            job.state = state;
        }
    }
}

JobTable::Job& JobTable::FindById(int id)
{
    for(auto& job : m_jobs_)
    {
        if(job.id == id)
        {
            return job;
        }
    }
    throw ShellException("no such job",1);
}
void JobTable::RemoveByID(int id)
{
    for (auto itr = m_jobs_.begin(); itr != m_jobs_.end(); ++itr)
    {
        if (itr->id == id)
        {
            m_jobs_.erase(itr);
            break;
        }
    }
}
} // namespace shell