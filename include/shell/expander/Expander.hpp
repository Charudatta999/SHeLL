#ifndef SHELL_EXPANDER_EXPANDER_HPP
#define SHELL_EXPANDER_EXPANDER_HPP

#include <memory>
#include <string>
#include <vector>

namespace shell
{
class ShellState;
namespace expander
{
std::vector<std::string> Expand(const std::string& word,
                                std::unique_ptr<ShellState>& state);
}
} // namespace shell
#endif // SHELL_EXPANDER_EXPANDER_HPP