#ifndef SHELL_EXPANDER_EXPANDER_HPP
#define SHELL_EXPANDER_EXPANDER_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace shell
{
    class ShellState;
namespace expander
{
using CommandRunner = std::function<std::string(const std::string&)>;
std::vector<std::string> Expand(const std::string& word,
                                std::unique_ptr<ShellState>& state,  const CommandRunner& cmdRunner);
}
} // namespace shell
#endif // SHELL_EXPANDER_EXPANDER_HPP