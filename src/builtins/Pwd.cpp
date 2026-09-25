#include "builtins/BuiltInFunction.hpp"
#include <unistd.h>
#include "io/FdOps.hpp"

namespace builtins
{

int Pwd(const std::vector<std::string>& /*argv*/,std::unique_ptr<BuiltinContext>& ctx)
{
    const auto& cwd = ctx->m_state_->GetCWD() + "\n";
    io::fdops::WriteAll(ctx->outFd, cwd);
    return 0;
}
}