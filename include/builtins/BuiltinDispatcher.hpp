#ifndef BUILTINS_BUILTIN_DISPATCHER
#define BUILTINS_BUILTIN_DISPATCHER

#include "builtins/BuiltInFunction.hpp"

#include <memory>
#include <unordered_map>

namespace builtins
{

using BuiltinFn = int (*)(const std::vector<std::string>& argv,
                          std::unique_ptr<BuiltinContext>& ctx);

class BuiltinDispatcher
{
public:
    BuiltinDispatcher();

    [[nodiscard]] bool IsBuiltin(const std::string& name) const;

    int Run(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx);

private:
    std::unordered_map<std::string, BuiltinFn> m_table_;
};
} // namespace builtins
#endif // BUILTINS_BUILTIN_DISPATCHER