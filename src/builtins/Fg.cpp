#include "builtins/BuiltInFunction.hpp"
#include "exec/SuspendedCoro.hpp"
#include "exec/WaitStatus.hpp"
#include "io/FdOps.hpp"
#include "shell/JobTable.hpp"
#include "shell/ShellException.hpp"
#include "utils/ErrorCodes.hpp"

#include <cctype>
#include <charconv>
#include <csignal>
#include <string>
#include <unistd.h>

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

// Bring a frozen compound (Ctrl-Z'd && / pipeline / loop) back to the
// foreground. fg owns only the process/terminal half here; the coroutine
// half lives behind job.suspended->resume, which the Executor built.
int ResumeSuspendedJob(
    shell::JobTable::Job job,
    const std::unique_ptr<shell::JobTable>& jobs,
    const std::unique_ptr<builtins::BuiltinContext>& ctx)
{
    tcsetpgrp(STDIN_FILENO, job.pid);
    kill(-job.pid, SIGCONT);
    // WUNTRACED: the leaf may finish OR be Ctrl-Z'd again — we must tell
    // those apart, so we cannot use a plain blocking wait.
    exec::WaitStatus status(job.pid, exec::WaitMode::Foreground);
    // Take the terminal back before driving the tail; the && tail runs in
    // the shell's own foreground.
    tcsetpgrp(STDIN_FILENO, getpgrp());

    if (status.IsStopped())
    {
        // The leaf stopped again before finishing — nothing to drive yet;
        // leave the job exactly as frozen as the user left it.
        jobs->UpdateJobState(job.id, shell::JobTable::State::Stopped);
        io::fdops::WriteAll(ctx->outFd,
                            "[" + std::to_string(job.id) +
                                "]+ Stopped " + job.command + "\n");
        return SIGNAL_EXIT_BASE + SIGTSTP;
    }

    // Leaf finished: hand its real status to the coroutine and let the
    // rest of the compound run.
    int leafStatus = status.Exited()
                         ? status.ExitCode()
                         : SIGNAL_EXIT_BASE + status.GetSignal();
    auto outcome = job.suspended->resume(leafStatus);
    io::fdops::WriteAll(ctx->outFd, job.command + " \n");

    if (outcome.completed)
    {
        jobs->RemoveByID(job.id);
        return outcome.status;
    }

    // Re-froze deeper in the compound: re-point the job at the new leaf
    // process group and report it stopped.
    shell::JobTable::Job& live = jobs->FindById(job.id);
    live.pid = outcome.newPgid;
    live.state = shell::JobTable::State::Stopped;
    io::fdops::WriteAll(ctx->outFd,
                        "[" + std::to_string(job.id) + "]+ Stopped " +
                            job.command + "\n");
    return SIGNAL_EXIT_BASE + SIGTSTP;
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
        auto currJobId = jobs->CurrentId();
        if (!currJobId.has_value())
        {
            std::string out = "fg: current: no such job \n";
            io::fdops::WriteAll(ctx->errFd, out);
            return 1;
        }
        auto job = jobs->FindById(currJobId.value());
        if (job.suspended)
            return ResumeSuspendedJob(job, jobs, ctx);
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
        if (job.suspended)
            return ResumeSuspendedJob(job, jobs, ctx);
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