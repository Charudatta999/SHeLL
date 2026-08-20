#ifndef SHELL_JOBTABLE_HPP
#define SHELL_JOBTABLE_HPP
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <sys/types.h>
#include <vector>
namespace exec
{
struct SuspendedCoro;
}

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
        std::shared_ptr<exec::SuspendedCoro> suspended;
    };

    JobTable();
    ~JobTable();
    JobTable(const JobTable&) = delete;
    JobTable& operator=(const JobTable&) = delete;
    JobTable(JobTable&&) = delete;
    JobTable& operator=(JobTable&&) = delete;

    std::vector<Job> Reap();

    int Add(pid_t pid,
            const std::string& command,
            State state = State::Running);
    int AddSuspended(pid_t pid,
                     const std::string& command,
                     std::shared_ptr<exec::SuspendedCoro> suspended,
                     State state = State::Stopped);
    [[nodiscard]]
    const std::vector<Job>& List() const;
    void UpdateJobState(int id, State state);
    [[nodiscard]]
    Job& FindById(int id);

    void RemoveByID(int id);

    /// @brief Drop every job (and any SuspendedCoro they own).
    /// Call while the Executor that those frames reference is still
    /// alive — coroutine destroy runs CompoundScope dtors that write
    /// back into it.
    void Clear();

    [[nodiscard]]
    std::optional<int> CurrentId() const;

    [[nodiscard]]
    std::optional<int> PreviousId() const;

    [[nodiscard]]
    Job& ResolveJobSpec(const std::string&);

    private : void Touch(int id);
    void DropFromRecency(int id);
    Job& MatchByCommand(const std::string& needle, bool substring);
    std::vector<Job> m_jobs_;
    int m_nextId_;
    std::vector<int> m_recency_;
};

} // namespace shell
#endif // SHELL_JOBTABLE_HPP