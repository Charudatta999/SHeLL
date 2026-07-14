#include "builtins/BuiltInFunction.hpp"

#include <string>
#include <unistd.h>

namespace builtins
{
namespace
{
void PrintExported(BuiltinContext& ctx)
{
    for (const auto& name : ctx.m_state_->GetExportedNames())
    {
        std::string line = "declare -x " + name;
        if (auto value = ctx.m_state_->GetVar(name))
            line += "=\"" + *value + "\"";
        line += "\n";
        write(ctx.outFd, line.c_str(), line.size());
    }
}
} // namespace

int Export(const std::vector<std::string>& argv,
           std::unique_ptr<BuiltinContext>& ctx)
{
    if (argv.size() == 1 || argv[1] == "-p")
    {
        PrintExported(*ctx);
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
                "export: `" + arg + "': not a valid identifier\n";
            write(ctx->errFd, err.c_str(), err.size());
            status = 1;
            continue;
        }
        if (eq != std::string::npos &&
            !ctx->m_state_->SetVar(name, arg.substr(eq + 1)))
        {
            std::string err =
                "export: " + name + ": readonly variable\n";
            write(ctx->errFd, err.c_str(), err.size());
            status = 1;
            continue;
        }
        ctx->m_state_->ExportVar(name);
    }
    return status;
}
} // namespace builtins
