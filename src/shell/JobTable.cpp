#include "shell/JobTable.hpp"

#include "exec/WaitStatus.hpp"
#include "shell/ShellException.hpp"

#include <algorithm>
#include <charconv>

namespace shell
{
JobTable::JobTable() : m_jobs_({}), m_nextId_(1) {}

JobTable::~JobTable() = default;

std::vector<JobTable::Job> JobTable::Reap()
{
    std::vector<Job> jobEvents;
    for (auto itr = m_jobs_.begin(); itr != m_jobs_.end();)
    {
        exec::WaitStatus waitStatus(itr->pid, exec::WaitMode::Poll);
        if (!waitStatus.IsRunning())
        {
            if (waitStatus.IsStopped())
            {
                itr->state = State::Stopped;
                jobEvents.push_back(*itr);
                Touch(itr->id);
                ++itr;
                continue;
            }
            itr->state = State::Done;
            jobEvents.push_back(*itr);
            DropFromRecency(itr->id);
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
                       .state = state,
                       .suspended = nullptr});
    Touch(jobId);
    return jobId;
}

const std::vector<JobTable::Job>& JobTable::List() const
{
    return m_jobs_;
}

void JobTable::UpdateJobState(int id, State state)
{
    for (auto& job : m_jobs_)
    {
        if (job.id == id)
        {
            job.state = state;
            Touch(id);
        }
    }
}

JobTable::Job& JobTable::FindById(int id)
{
    for (auto& job : m_jobs_)
    {
        if (job.id == id)
        {
            return job;
        }
    }
    throw ShellException("no such job", 1);
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
    DropFromRecency(id);
}

int JobTable::AddSuspended(
    pid_t pid,
    const std::string& command,
    std::shared_ptr<exec::SuspendedCoro> suspended,
    State state)
{
    int jobId = m_nextId_++;
    m_jobs_.push_back({.id = jobId,
                       .pid = pid,
                       .command = command,
                       .state = state,
                       .suspended = std::move(suspended)});
    Touch(jobId);
    return jobId;
}

void JobTable::Touch(int id)
{
    auto itr = std::ranges::find(m_recency_, id);

    if (itr != m_recency_.end())
    {
        // Found
        DropFromRecency(id);
    }
    m_recency_.push_back(id);
}

void JobTable::DropFromRecency(int id)
{
    auto itr = std::ranges::find(m_recency_, id);
    if (itr != m_recency_.end())
    {
        m_recency_.erase(itr);
    }
}

std::optional<int> JobTable::CurrentId() const
{
    return m_recency_.empty() ? std::nullopt
                              : std::make_optional(m_recency_.back());
}

std::optional<int> JobTable::PreviousId() const
{
    if (m_recency_.size() < 2)
    {
        return std::nullopt;
    }
    return std::make_optional(m_recency_[m_recency_.size() - 2]);
}

JobTable::Job& JobTable::ResolveJobSpec(const std::string& cmd)
{
    try
    {

        if (cmd == "%" || cmd == "%%" || cmd == "%+" || cmd.empty())
        {
            if(!CurrentId().has_value())
            {
                throw ShellException("no such job", 1);
            }
            return FindById(CurrentId().value());
        }
        if (cmd == "%-")
        {
            if(!PreviousId().has_value())
            {
                throw ShellException("no such job", 1);
            }
            return FindById(PreviousId().value());
        }
        std::string spec = cmd;
        if (!spec.empty() && spec[0] == '%')
            spec.erase(0, 1);
        int value = 0;
        auto [ptr, ec] = std::from_chars(spec.data(),
                                         spec.data() + spec.size(),
                                         value);
        if (ec != std::errc{} || ptr != spec.data() + spec.size())
            throw ShellException("no such job", 1);
        return FindById(value);
    }
    catch (const ShellException&)
    {
        throw ShellException("no such job", 1);
    }
}
} // namespace shell