#include "builtins/BuiltInFunction.hpp"

#include <climits>
#include <optional>
#include <string>
#include <unistd.h>

namespace builtins
{
int Cd(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx)
{
    std::string target{};
    std::string errStr{};
    if (argv.size() < 2)
    {
        auto home = ctx->m_state_->GetVar("HOME");
        if (!home.has_value())
        {
            errStr = "HOME path varibale not set ";
            write(ctx->errFd, errStr.c_str(), errStr.size());
            return 1;
        }
        target = home.value();
    }
    else
    {
        target = argv[1];
    }

    const std::string oldPwd = ctx->m_state_->GetCWD();

    if (chdir(target.c_str()) != 0)
    {
        errStr = "cd: " + target + "\n";
        write(ctx->errFd, errStr.c_str(), errStr.size());
        return 1;
    }

    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf)))
    {
        ctx->m_state_->SetCWD(buf);
        ctx->m_state_->SetVar("OLDPWD", oldPwd);
        ctx->m_state_->SetVar("PWD", buf);
        // Bash keeps PWD/OLDPWD in the child environment.
        ctx->m_state_->ExportVar("OLDPWD");
        ctx->m_state_->ExportVar("PWD");
    }
    return 0;
}
} // namespace builtins