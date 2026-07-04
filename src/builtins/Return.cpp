#include "builtins/BuiltInFunction.hpp"
#include "exec/ControlFlow.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace builtins
{
int Return(const std::vector<std::string>& argv,
           std::unique_ptr<BuiltinContext>& ctx)
{
    int status = ctx->m_state_->GetLastCommandExitCode();
    if (argv.size() >= 2)
    {
        try
        {
            std::size_t pos = 0;
            status = std::stoi(argv[1], &pos);
            if (pos != argv[1].size())
                throw std::invalid_argument(argv[1]);
        }
        catch (const std::exception&)
        {
            const std::string err = "return: " + argv[1] +
                                    ": numeric argument required\n";
            write(ctx->errFd, err.c_str(), err.size());
            return 255;
        }
    }
    throw exec::FunctionReturn{status};
}
} // namespace builtins
