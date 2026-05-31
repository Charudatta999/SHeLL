#include "exec/Executor.hpp"
#include "exec/ExecHelpers.hpp"
#include "exec/Pipeline.hpp"
#include "builtins/BuiltinDispatcher.hpp"
#include "parser/Command.hpp"
#include "shell/ShellState.hpp"

namespace exec
{
Executor::Executor(std::unique_ptr<shell::ShellState>& state,
                   std::unique_ptr<builtin::BuiltinDispatcher>& builtins)
    : m_state_(state)
    , m_builtins_(builtins)
{
}

CommandSpec Executor::BuildSpec(const parser::SimpleCommand& command) const
{
    return CommandSpec(command.argv,command.redirects);
}
int Executor::Exec(const parser::AstNode& n)
{
    auto pipeline = n.line;
    Pipeline().Run(pipeline)
    return 0;
}

int Executor::ExecList(const parser::ListNode&)
{
    return 0;
}

int Executor::ExecAndOr(const parser::AndOrNode&) {}

int Executor::ExecPipeline(const parser::PipelineNode&  pipeLine) {}

int Executor::ExecSimple(const parser::SimpleCommand&) {}

} // namespace exec