#include "builtins/BuiltInFunction.hpp"

#include "io/FdOps.hpp"
#include "shell/JobTable.hpp"
#include <string>
#include <unistd.h>

namespace builtins {
int Jobs(const std::vector<std::string>& /*argv*/, std::unique_ptr<BuiltinContext>& ctx)
{
    const auto& jobs = ctx->m_state_->GetJobs()->List();
    for (const auto& job : jobs)
    {
        std::string line = "[" + std::to_string(job.id) + "] "
                         + (job.state == shell::JobTable::State::Running ? "Running" : "Stopped") + "  "
                         + job.command + "\n";
        io::fdops::WriteAll(ctx->outFd, line);
    }
    return 0;
}
}