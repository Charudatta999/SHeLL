#include "builtins/BuiltInFunction.hpp"
#include "exec/Process.hpp"

namespace builtin
{
    int Cd(const std::vector<std::string> &argv, std::unique_ptr<BuiltinContext> &ctx)
    {
        const auto process = exec::Process();
        process
    }

}