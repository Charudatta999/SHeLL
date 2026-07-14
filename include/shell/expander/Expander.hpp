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
                                std::unique_ptr<ShellState>& state,  const CommandRunner& cmdRunner,bool assignment = false);

// Purely textual, stateless brace expansion. Runs before every other
// expansion. One word in, >= 1 word out (a word with no valid brace
// group comes back unchanged). Handles comma lists {a,b}, numeric/char
// ranges {1..5} {a..e} with optional step {1..9..2}, zero-padded ranges
// {01..10}, nesting {a,b{c,d}}, cross products x{a,b}{1,2}, and no-match
// passthrough {abc}.
std::vector<std::string> BraceExpand(const std::string& word);
}
} // namespace shell
#endif // SHELL_EXPANDER_EXPANDER_HPP
