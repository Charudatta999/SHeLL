#include "builtins/BuiltInFunction.hpp"

#include <unistd.h>

namespace builtins
{
// Minimal echo: join argv[1..] with spaces, trailing newline.
// Flags (-n, -e, -E) and escape sequences: see issue #6.
int Echo(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx)
{
    for (std::size_t i = 1; i < argv.size(); ++i)
    {
        if (i > 1)
        {
            write(ctx->outFd, " ", 1);
        }
        write(ctx->outFd, argv[i].c_str(), argv[i].size());
    }
    write(ctx->outFd, "\n", 1);
    return 0;
}
} // namespace builtins
