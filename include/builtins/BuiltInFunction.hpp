#ifndef BUILTIN_BUILTIN_FUNCTION_HPP
#define BUILTIN_BUILTIN_FUNCTION_HPP

#include "shell/ShellState.hpp"

#include <memory>
#include <vector>

namespace builtin
{
struct BuiltinContext
{
    std::unique_ptr<shell::ShellState>& m_state_;
    int inFd = 0;
    int outFd = 1;
    int errFd = 2;
};

int Cd(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx);
int Echo(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx);
int Exit(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx);
int Pwd(const std::vector<std::string>& argv, std::unique_ptr<BuiltinContext>& ctx);
} // namespace builtin
#endif // BUILTIN_BUILTIN_FUNCTION_HPP