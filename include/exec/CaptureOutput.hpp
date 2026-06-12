#ifndef EXEC_CAPTUREOUTPUT_HPP
#define EXEC_CAPTUREOUTPUT_HPP

#include "shell/expander/Expander.hpp"

#include <memory>
#include <string>

namespace parser::ast
{
class AstNode;
}

namespace shell
{
class ShellState;
}

namespace builtins
{
class BuiltinDispatcher;
}

namespace exec
{

std::string CaptureOutput(
    const std::unique_ptr<parser::ast::AstNode>& root,
    std::unique_ptr<shell::ShellState>& state,
    std::unique_ptr<builtins::BuiltinDispatcher>& dispatcher,
    const shell::expander::CommandRunner& cmdRunner);

} // namespace exec
#endif // EXEC_CAPTUREOUTPUT_HPP