#include "builtins/BuiltInFunction.hpp"
#include "exec/ControlFlow.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace builtins
{
int Continue(const std::vector<std::string>& argv,
             std::unique_ptr<BuiltinContext>& ctx)
{
    int level = 1;
    if (argv.size() >= 2)
    {
        try
        {
            std::size_t pos = 0;
            level = std::stoi(argv[1], &pos);
            if (pos != argv[1].size())
                throw std::invalid_argument(argv[1]);
        }
        catch (const std::exception&)
        {
            const std::string err = "continue: " + argv[1] +
                                    ": numeric argument required\n";
            write(ctx->errFd, err.c_str(), err.size());
            return 128;
        }
        if (level < 1)
        {
            const std::string err =
                "continue: " + argv[1] + ": loop count out of range\n";
            write(ctx->errFd, err.c_str(), err.size());
            return 1;
        }
    }
    throw exec::LoopControl{exec::LoopControl::Kind::Continue, level};
}
} // namespace builtins
