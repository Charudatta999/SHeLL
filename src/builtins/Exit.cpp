#include "builtins/BuiltInFunction.hpp"

#include <cstdlib>
#include <string>
#include <unistd.h>

namespace builtins
{
// exit [code] — request shell shutdown. Code defaults to last command's
// exit status when omitted; explicit non-numeric arg is an error.
int Exit(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx)
{
    int code = ctx->m_state_->GetLastCommandExitCode();
    if (argv.size() >= 2)
    {
        try
        {
            code = std::stoi(argv[1]);
        }
        catch (const std::exception&)
        {
            const std::string err = "exit: " + argv[1] + ": numeric argument required\n";
            write(ctx->errFd, err.c_str(), err.size());
            code = 2;
        }
    }
    ctx->m_state_->RequestExit(code);
    return code;
}
} // namespace builtins
