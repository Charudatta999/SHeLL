#include "builtins/BuiltInFunction.hpp"
#include "shell/JobTable.hpp"   // List() returns const vector<Job>&
#include <string>
#include <unistd.h>

namespace builtins {
int Jobs(const std::vector<std::string>& /*argv*/, std::unique_ptr<BuiltinContext>& ctx)
{
    const auto& jobs = ctx->m_state_->GetJobs()->List();
    for (const auto& job : jobs)
    {
        std::string line = "[" + std::to_string(job.id) + "] "
                         + (job.running ? "Running" : "Done") + "  "
                         + job.command + "\n";
        write(ctx->outFd, line.c_str(), line.size());
    }
    return 0;
}
}