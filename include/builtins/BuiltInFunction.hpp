#ifndef BUILTIN_BUILTIN_FUNCTION_HPP
#define BUILTIN_BUILTIN_FUNCTION_HPP

#include "shell/ShellState.hpp"

#include <memory>
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
} // namespace builtins
#endif // BUILTIN_BUILTIN_FUNCTION_HPP