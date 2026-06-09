#include "shell/ShellArithmeticVars.hpp"

#include "arithmetic/ArithmeticVars.hpp"
#include "shell/ShellState.hpp"

#include <memory>
#include <string>

namespace shell
{
ShellArithmeticVars::ShellArithmeticVars(
    std::unique_ptr<ShellState>& state)
    : m_state_(state)
{
}

std::optional<std::string>
ShellArithmeticVars::Get(const std::string& var) const
{
    return m_state_->GetVar(var);
}

void ShellArithmeticVars::Set(const std::string& varName, const std::string& value)
{
    m_state_->SetVar(varName,value);
}
} // namespace shell