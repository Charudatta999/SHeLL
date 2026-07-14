#include "builtins/BuiltInFunction.hpp"

#include <string>
#include <unistd.h>

namespace builtins
{
int Unset(const std::vector<std::string>& argv,
          std::unique_ptr<BuiltinContext>& ctx)
{
    bool varsOnly = false;
    bool funcsOnly = false;
    std::size_t i = 1;
    for (; i < argv.size() && argv[i].size() == 2 && argv[i][0] == '-';
         ++i)
    {
        if (argv[i] == "-v")
            varsOnly = true;
        else if (argv[i] == "-f")
            funcsOnly = true;
        else
        {
            std::string err =
                "unset: " + argv[i] + ": invalid option\n";
            write(ctx->errFd, err.c_str(), err.size());
            return 2;
        }
    }

    int status = 0;
    for (; i < argv.size(); ++i)
    {
        const auto& name = argv[i];
        if (!funcsOnly)
        {
            if (ctx->m_state_->IsReadonly(name))
            {
                std::string err =
                    "unset: " + name +
                    ": cannot unset: readonly variable\n";
                write(ctx->errFd, err.c_str(), err.size());
                status = 1;
                continue;
            }
            bool hadVar = ctx->m_state_->GetVar(name).has_value();
            ctx->m_state_->UnSetVar(name);
            if (hadVar || varsOnly)
                continue;
        }
        ctx->m_state_->UnsetFunction(name);
    }
    return status;
}
} // namespace builtins
