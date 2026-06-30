#include "builtins/BuiltInFunction.hpp"
#include "io/FdOps.hpp"
#include "shell/JobTable.hpp"
#include "shell/ShellException.hpp"

#include <charconv>
#include <csignal>
#include <string>

namespace
{
bool ResumeInBackground(const shell::JobTable::Job& job)
{
    return (kill(-job.pid, SIGCONT) == 0);
}
} // namespace

namespace builtins
{
int Bg(const std::vector<std::string>& argv,
       std::unique_ptr<BuiltinContext>& ctx)
{
    const auto& jobs = ctx->m_state_->GetJobs();
    if (jobs->List().empty())
    {
        std::string out = "bg: current: no such job \n";
        io::fdops::WriteAll(ctx->errFd, out);
        return 1;
    }
    if (argv.size() == 1 && argv[0] == "bg" && !jobs->List().empty())
    {

        auto currJobId = jobs->CurrentId();
        if (!currJobId.has_value())
        {
            std::string out = "bg: current: no such job \n";
            io::fdops::WriteAll(ctx->errFd, out);
            return 1;
        }
        auto job = jobs->FindById(currJobId.value());
        if (ResumeInBackground(job))
        {
            jobs->UpdateJobState(currJobId.value(),
                                 shell::JobTable::State::Running);
            io::fdops::WriteAll(
                ctx->outFd,
                "[" + std::to_string(currJobId.value()) + "] " +
                    job.command +
                    " &\n");
        }
        else
        {
            std::string out = "bg: error: failed to start job : " +
                              std::to_string(currJobId.value());
            io::fdops::WriteAll(ctx->errFd, out);
            return 1;
        }
        return 0;
    }
    for (size_t index = 1; index < argv.size(); index++)
    {
        auto cmd = argv[index];

        if (cmd[0] == '%')
        {
            cmd.erase(0, 1);
        }
        int value = 0;
        auto [ptr, ec] = std::from_chars(cmd.data(),
                                         cmd.data() + cmd.size(),
                                         value);
        if (ec != std::errc{} || ptr != cmd.data() + cmd.size())
        {
            std::string out =
                "bg: warning:" + cmd + ": no such job\n";
            io::fdops::WriteAll(ctx->errFd, out);
            continue;
        }
        try
        {
            auto job = jobs->FindById(value);
            if (ResumeInBackground(job))
            {
                jobs->UpdateJobState(job.id,
                                     shell::JobTable::State::Running);
                io::fdops::WriteAll(ctx->outFd,
                                    "[" + std::to_string(job.id) +
                                        "] " + job.command + " &\n");
            }
            else
            {
                std::string out =
                    "bg: error: failed to start job : " +
                    std::to_string(value) + "\n";
                io::fdops::WriteAll(ctx->errFd, out);
            }
        }
        catch (shell::ShellException& ex)
        {
            std::string out = "bg: error:" + cmd + ex.what() + "\n";
            io::fdops::WriteAll(ctx->errFd, out);
            continue;
        }
    }
    return 0;
}
} // namespace builtins