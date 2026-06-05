#include "builtins/BuiltInFunction.hpp"
#include <unistd.h>

namespace builtins
{

int Pwd(const std::vector<std::string>& /*argv*/,std::unique_ptr<BuiltinContext>& ctx)
{
    const auto& cwd = ctx->m_state_->GetCWD() + "\n";
    write(ctx->outFd,cwd.c_str(),cwd.size());
    return 0;
}
}