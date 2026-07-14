#include "builtins/BuiltInFunction.hpp"

#include <string>
#include <unistd.h>
#include <vector>

namespace builtins
{
namespace
{
// Canonical long names shared with IsOptionEnabled() consumers
// (the executor already checks "pipefail").
const char* LongOptionName(char shortOpt)
{
    switch (shortOpt)
    {
    case 'e':
        return "errexit";
    case 'x':
        return "xtrace";
    case 'u':
        return "nounset";
    default:
        return nullptr;
    }
}

void PrintAllVars(BuiltinContext& ctx)
{
    auto vars = ctx.m_state_->GetLocalVars();
    auto env = ctx.m_state_->GetEnv();
    vars.insert(env.begin(), env.end());
    for (const auto& entry : vars)
    {
        std::string line = entry.first + "=" + entry.second + "\n";
        write(ctx.outFd, line.c_str(), line.size());
    }
}
} // namespace

int Set(const std::vector<std::string>& argv,
        std::unique_ptr<BuiltinContext>& ctx)
{
    if (argv.size() == 1)
    {
        PrintAllVars(*ctx);
        return 0;
    }

    bool sawSeparator = false;
    std::size_t i = 1;
    for (; i < argv.size(); ++i)
    {
        const auto& arg = argv[i];
        if (arg == "--")
        {
            sawSeparator = true;
            ++i;
            break;
        }
        if (arg == "-o" || arg == "+o")
        {
            if (i + 1 >= argv.size())
            {
                std::string err =
                    "set: " + arg + ": option name required\n";
                write(ctx->errFd, err.c_str(), err.size());
                return 2;
            }
            ++i;
            if (arg[0] == '-')
                ctx->m_state_->SetOption(argv[i]);
            else
                ctx->m_state_->DisableOption(argv[i]);
            continue;
        }
        if (arg.size() >= 2 && (arg[0] == '-' || arg[0] == '+'))
        {
            for (std::size_t j = 1; j < arg.size(); ++j)
            {
                const char* longName = LongOptionName(arg[j]);
                if (!longName)
                {
                    std::string err = std::string("set: ") + arg[0] +
                                      arg[j] + ": invalid option\n";
                    write(ctx->errFd, err.c_str(), err.size());
                    return 2;
                }
                if (arg[0] == '-')
                    ctx->m_state_->SetOption(longName);
                else
                    ctx->m_state_->DisableOption(longName);
            }
            continue;
        }
        break; // first operand: it and the rest become $1..
    }

    // `set -- ...` replaces positionals even when empty (clears);
    // `set -e` alone must leave them untouched.
    if (sawSeparator || i < argv.size())
        ctx->m_state_->SetPositionalParams(
            std::vector<std::string>(argv.begin() +
                                         static_cast<long>(i),
                                     argv.end()));
    return 0;
}
} // namespace builtins
