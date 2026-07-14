#ifndef BUILTIN_BUILTIN_FUNCTION_HPP
#define BUILTIN_BUILTIN_FUNCTION_HPP

#include "shell/ShellState.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <vector>

namespace builtins
{
struct BuiltinContext
{
    std::unique_ptr<shell::ShellState>& m_state_;
    int inFd = 0;
    int outFd = 1;
    int errFd = 2;

    BuiltinContext(std::unique_ptr<shell::ShellState>& state)
        : m_state_(state)
    {
    }
};

/// @brief Test whether a name is a valid shell identifier:
///        [A-Za-z_][A-Za-z0-9_]*.
inline bool IsValidVarName(const std::string& name)
{
    if (name.empty())
        return false;
    auto first = static_cast<unsigned char>(name[0]);
    if (!std::isalpha(first) && first != '_')
        return false;
    return std::all_of(name.begin() + 1,
                       name.end(),
                       [](unsigned char chr)
                       { return std::isalnum(chr) || chr == '_'; });
}

int Cd(const std::vector<std::string>& argv,
       std::unique_ptr<BuiltinContext>& ctx);

[[maybe_unused]]
int Echo(const std::vector<std::string>& argv,
         std::unique_ptr<BuiltinContext>& ctx);

int Exit(const std::vector<std::string>& argv,
         std::unique_ptr<BuiltinContext>& ctx);

[[maybe_unused]]
int Pwd(const std::vector<std::string>& argv,
        std::unique_ptr<BuiltinContext>& ctx);

int Jobs(const std::vector<std::string>& /*argv*/,
         std::unique_ptr<BuiltinContext>& ctx);

int Fg(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx);

int Bg(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx);

int Wait(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx);

int Break(const std::vector<std::string>& argv,
          std::unique_ptr<BuiltinContext>& ctx);

int Continue(const std::vector<std::string>& argv,
             std::unique_ptr<BuiltinContext>& ctx);

int Return(const std::vector<std::string>& argv,
           std::unique_ptr<BuiltinContext>& ctx);

int Export(const std::vector<std::string>& argv,
           std::unique_ptr<BuiltinContext>& ctx);

int Unset(const std::vector<std::string>& argv,
          std::unique_ptr<BuiltinContext>& ctx);

int Set(const std::vector<std::string>& argv,
        std::unique_ptr<BuiltinContext>& ctx);

int Readonly(const std::vector<std::string>& argv,
             std::unique_ptr<BuiltinContext>& ctx);
} // namespace builtins
#endif // BUILTIN_BUILTIN_FUNCTION_HPP