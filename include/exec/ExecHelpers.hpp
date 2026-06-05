#ifndef EXEC_EXEC_HELPERS_HPP
#define EXEC_EXEC_HELPERS_HPP

#include "parser/ast/Redirect.hpp"

#include <string>
#include <utility>
#include <vector>

namespace exec
{
struct CommandSpec
{
    std::vector<std::string> argv;
    std::vector<parser::ast::Redirect> redirects;
    std::vector<std::pair<std::string, std::string>> envOverrides;

    CommandSpec(const std::vector<std::string>& argsArr)
        : argv{argsArr}
    {
    }
    CommandSpec(const std::vector<std::string>& argsArr, const std::vector<parser::ast::Redirect>& redirectsvtr)
        : argv{argsArr}
        , redirects{redirectsvtr}
    {
    }
    CommandSpec(const std::vector<std::string>& argsArr,
        const std::vector<parser::ast::Redirect>& redirectsvtr,
        const std::vector<std::pair<std::string, std::string>>& envOverridesVtr)
        : argv{argsArr}
        , redirects{redirectsvtr}
        , envOverrides{envOverridesVtr}
    {
    }
};
} // namespace exec
#endif // EXEC_EXEC_HELPERS_HPP