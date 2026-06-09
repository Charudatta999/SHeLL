#ifndef SHELL_SHELLARITHMETICVARS_HPP
#define SHELL_SHELLARITHMETICVARS_HPP

#include "arithmetic/ArithmeticVars.hpp"
#include "shell/ShellState.hpp"

#include <memory>

namespace shell
{

class ShellState;

class ShellArithmeticVars final : public arithmetic::ArithmeticVars
{
public:
    ShellArithmeticVars(std::unique_ptr<ShellState>& state);
    ~ShellArithmeticVars() = default;
    ShellArithmeticVars(const ShellArithmeticVars&) = delete;
    ShellArithmeticVars&
    operator=(const ShellArithmeticVars&) = delete;
    ShellArithmeticVars(ShellArithmeticVars&&) = delete;
    ShellArithmeticVars& operator=(ShellArithmeticVars&&) = delete;

    [[nodiscard]] std::optional<std::string>
    Get(const std::string&) const override;
    void Set(const std::string&, const std::string&) override;

private:
    std::unique_ptr<ShellState>& m_state_;
};

} // namespace shell
#endif // SHELL_SHELLARITHMETICVARS_HPP