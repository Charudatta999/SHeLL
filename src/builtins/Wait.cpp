#include "builtins/BuiltInFunction.hpp"
#include "exec/WaitStatus.hpp"
#include "io/FdOps.hpp"
#include "shell/JobTable.hpp"
#include "shell/ShellException.hpp"
#include "utils/ErrorCodes.hpp"

#include <charconv>
#include <csignal>
#include <string>
#include <vector>

namespace
{
// Block until the job terminates (stops ignored, UntilExit).
// Returns its status: exit code, or 128+signal if killed.
int AwaitJob(const shell::JobTable::Job& job)
{
    exec::WaitStatus status(job.pid, exec::WaitMode::UntilExit);
    if (status.Signaled())
        return SIGNAL_EXIT_BASE + status.GetSignal();
    if (status.Exited())
        return status.ExitCode();
    return INVALID_STATUS;
}
} // namespace

namespace builtins
{
int Wait(const std::vector<std::string>& argv,
         std::unique_ptr<BuiltinContext>& ctx)
{
    const auto& jobs = ctx->m_state_->GetJobs();

    // No argument: wait for every job, then return 0.
    if (argv.size() == 1)
    {
        // Snapshot ids first , RemoveByID mutates the list.
        std::vector<int> ids;
        for (const auto& job : jobs->List())
            ids.push_back(job.id);

        for (int id : ids)
        {
            try
            {
                AwaitJob(jobs->FindById(id));
                jobs->RemoveByID(id);
            }
            catch (const shell::ShellException&)
            {
                // job vanished between snapshot and wait , skip
            }
        }
        return 0;
    }

    // wait %id / wait id: block on each, $? = the last one's status.
    int last = 0;
    for (std::size_t index = 1; index < argv.size(); ++index)
    {
        std::string spec = argv[index];
        if (!spec.empty() && spec[0] == '%')
            spec.erase(0, 1);

        int id = 0;
        auto [ptr, ec] =
            std::from_chars(spec.data(), spec.data() + spec.size(), id);
        if (ec != std::errc{} || ptr != spec.data() + spec.size())
        {
            std::string out =
                "wait: " + argv[index] + ": no such job\n";
            io::fdops::WriteAll(ctx->errFd, out);
            last = 1;
            continue;
        }

        try
        {
            last = AwaitJob(jobs->FindById(id));
            jobs->RemoveByID(id);
        }
        catch (const shell::ShellException&)
        {
            std::string out =
                "wait: " + argv[index] + ": no such job\n";
            io::fdops::WriteAll(ctx->errFd, out);
            last = 1;
        }
    }
    return last;
}
} // namespace builtins
