#include "builtins/BuiltInFunction.hpp"

#include <string>
#include <unistd.h>

namespace builtins
{
namespace
{
void PrintReadonly(BuiltinContext& ctx)
{
    for (const auto& name : ctx.m_state_->GetReadonlyVars())
    {
        std::string line = "readonly " + name;
        if (auto value = ctx.m_state_->GetVar(name))
            line += "=\"" + *value + "\"";
        line += "\n";
        write(ctx.outFd, line.c_str(), line.size());
    }
}
} // namespace

int Readonly(const std::vector<std::string>& argv,
             std::unique_ptr<BuiltinContext>& ctx)
{
    if (argv.size() == 1 || argv[1] == "-p")
    {
        PrintReadonly(*ctx);
        return 0;
    }

    int status = 0;
    for (std::size_t i = 1; i < argv.size(); ++i)
    {
        const auto& arg = argv[i];
        auto eq = arg.find('=');
        auto name = eq == std::string::npos ? arg : arg.substr(0, eq);
        if (!IsValidVarName(name))
        {
            std::string err =
                "readonly: `" + arg + "': not a valid identifier\n";
            write(ctx->errFd, err.c_str(), err.size());
            status = 1;
            continue;
        }
        if (eq != std::string::npos &&
            !ctx->m_state_->SetVar(name, arg.substr(eq + 1)))
        {
            std::string err =
                "readonly: " + name + ": readonly variable\n";
            write(ctx->errFd, err.c_str(), err.size());
            status = 1;
            continue;
        }
        ctx->m_state_->MarkReadonly(name);
    }
    return status;
}
} // namespace builtins
