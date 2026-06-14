#include "builtins/BuiltInFunction.hpp"
#include "exec/WaitStatus.hpp"
#include "io/FdOps.hpp"
#include "shell/JobTable.hpp"
#include "shell/ShellException.hpp"
#include "utils/ErrorCodes.hpp"

#include <cctype>
#include <charconv>
#include <csignal>
#include <string>

namespace
{
bool Resume(const shell::JobTable::Job& job)
{
    return (kill(-job.pid, SIGCONT) == 0);
}

int HandleForegroundOutcome(
    exec::WaitStatus& status,
    shell::JobTable::Job job,
    const std::unique_ptr<shell::JobTable>& jobs,
    const std::unique_ptr<builtins::BuiltinContext>& ctx)
{
    tcsetpgrp(STDIN_FILENO, getpgrp());
    if (status.IsStopped())
    {
        jobs->UpdateJobState(job.id, shell::JobTable::State::Stopped);
        io::fdops::WriteAll(ctx->outFd,
                            "[" + std::to_string(job.id) + "]+ " +
                                "Stopped " + job.command + " \n");

        return SIGNAL_EXIT_BASE + SIGTSTP;
    }
    if (status.Signaled())
    {
        jobs->RemoveByID(job.id);
        return SIGNAL_EXIT_BASE + status.GetSignal();
    }
    if (status.Exited())
    {
        jobs->RemoveByID(job.id);
        return status.ExitCode();
    }
    return 1;
}
} // namespace

namespace builtins
{
int Fg(const std::vector<std::string>& argv,
       std::unique_ptr<BuiltinContext>& ctx)
{
    const auto& jobs = ctx->m_state_->GetJobs();
    if (jobs->List().empty())
    {
        std::string out = "fg: no background jobs \n";
        io::fdops::WriteAll(ctx->errFd, out);
        return 1;
    }

    if (argv.size() == 1 && argv[0] == "fg")
    {
        auto job = jobs->List().back();
        tcsetpgrp(STDIN_FILENO, job.pid);
        if (Resume(job))
        {
            exec::WaitStatus status(job.pid,
                                    exec::WaitMode::Foreground);
            int res = HandleForegroundOutcome(status, job, jobs, ctx);
            io::fdops::WriteAll(ctx->outFd, job.command + " \n");

            return res;
        }
        else
        {
            std::string out = "fg: error: failed to start job : " +
                              std::to_string(job.id);
            io::fdops::WriteAll(ctx->errFd, out);
            return 1;
        }
    }
    std::string cmd = argv[1];
    int res = 1;
    if (!cmd.empty() && cmd[0] == '%')
    {
        cmd.erase(0, 1);
    }
    int value = 0;
    auto [ptr, ec] =
        std::from_chars(cmd.data(), cmd.data() + cmd.size(), value);
    if (ec != std::errc{} || ptr != cmd.data() + cmd.size())
    {
        std::string out = "fg: warning:" + cmd + ": no such job\n";
        io::fdops::WriteAll(ctx->errFd, out);
        return 1;
    }

    try
    {
        auto job = jobs->FindById(value);
        if (Resume(job))
        {

            exec::WaitStatus status(job.pid, exec::WaitMode::UntilExit);
            res = HandleForegroundOutcome(status, job, jobs, ctx);
            io::fdops::WriteAll(ctx->outFd, job.command + " \n");
        }
        else
        {
            std::string out =
                "fg: error: failed to start job : " + job.command +
                "\n";
            io::fdops::WriteAll(ctx->errFd, out);
        }
    }
    catch (shell::ShellException& ex)
    {
        std::string out = "fg: error: " + cmd + ex.what() + "\n";
        io::fdops::WriteAll(ctx->errFd, out);
    }
    return res;
}
} // namespace builtins