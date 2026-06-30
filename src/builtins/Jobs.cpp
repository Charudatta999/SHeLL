#include "builtins/BuiltInFunction.hpp"

#include "io/FdOps.hpp"
#include "shell/JobTable.hpp"
#include <string>
#include <unistd.h>

namespace builtins {
int Jobs(const std::vector<std::string>& /*argv*/, std::unique_ptr<BuiltinContext>& ctx)
{
    const auto& jobs = ctx->m_state_->GetJobs()->List();
    if (jobs.empty())
    {
        std::string out = "jobs: no background jobs \n";
        io::fdops::WriteAll(ctx->errFd, out);
        return 1;
    }

    int currJob = -1;
    int prevJob = -1;
    if (ctx->m_state_->GetJobs()->CurrentId().has_value())
    {
        currJob =ctx->m_state_->GetJobs()->CurrentId().value();
    }
    if (ctx->m_state_->GetJobs()->PreviousId().has_value())
    {
        prevJob = ctx->m_state_->GetJobs()->PreviousId().value();
    }
    for (const auto& job : jobs)
    {
        std::string marker = " ";
        if(currJob == job.id)
        {
            marker = "+";
        }
        else if(prevJob == job.id)
        {
            marker = "-";
        }
        std::string line =
            "[" + std::to_string(job.id) + "]" + marker + " " +
            (job.state == shell::JobTable::State::Running
                 ? "Running"
                 : "Stopped") +
            "  " + job.command + "\n";
        io::fdops::WriteAll(ctx->outFd, line);
    }

    return 0;
}
}