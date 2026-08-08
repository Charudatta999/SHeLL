#ifndef EXEC_PROCESSSUBSTITUTION_HPP
#define EXEC_PROCESSSUBSTITUTION_HPP

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

// Starts `root` connected to a fresh pipe and returns the /dev/fd/N
// path for the parent-side end, for splicing into the word in place
// of <(...) / >(...).
//   writeMode == false (<(cmd)): child's stdout feeds the pipe, the
//     parent keeps the read end — the outer command reads from it.
//   writeMode == true  (>(cmd)): the pipe feeds the child's stdin,
//     the parent keeps the write end — the outer command writes to
//     it.
// The kept fd and the child's pid are registered on `state` so the
// caller can close the fd and reap the child once the foreground
// command has been launched (see ShellState::TakeProcSubs).
std::string StartProcessSub(
    const std::shared_ptr<parser::ast::AstNode>& root,
    std::unique_ptr<shell::ShellState>& state,
    std::unique_ptr<builtins::BuiltinDispatcher>& dispatcher,
    const shell::expander::CommandRunner& cmdRunner,
    bool writeMode);

// Shared binder used by Repl, CaptureOutput, and nested proc-sub
// bodies: tokenize → parse → StartProcessSub.
shell::expander::ProcSubRunner MakeProcSubRunner(
    std::unique_ptr<shell::ShellState>& state,
    std::unique_ptr<builtins::BuiltinDispatcher>& dispatcher,
    const shell::expander::CommandRunner& cmdRunner);

} // namespace exec
#endif // EXEC_PROCESSSUBSTITUTION_HPP
