#ifndef BUILTINS_BUILTIN_DISPATCHER
#define BUILTINS_BUILTIN_DISPATCHER

#include "builtins/BuiltInFunction.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace builtins
{

using BuiltinFn = int (*)(const std::vector<std::string>& argv,
                          std::unique_ptr<BuiltinContext>& ctx);

class BuiltinDispatcher
{
public:
    BuiltinDispatcher();

    [[nodiscard]] bool IsBuiltin(const std::string& name) const;

    /// @brief All builtin names, for completion.
    [[nodiscard]] std::vector<std::string> Names() const;

    /// @brief One-line description of a builtin, or "" if unknown.
    [[nodiscard]] std::string Description(const std::string& name) const;

    int Run(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx);

private:
    std::unordered_map<std::string, BuiltinFn> m_table_;
    std::unordered_map<std::string, std::string> m_descriptions_;
};
} // namespace builtins
#endif // BUILTINS_BUILTIN_DISPATCHER