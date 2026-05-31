#ifndef EXEC_EXECUTOR_HPP
#define EXEC_EXECUTOR_HPP

#include "exec/ExecHelpers.hpp"

#include <memory>
#include <string>
#include <vector>

namespace builtin
{
    class BuiltinDispatcher;
}
namespace shell
{
    class ShellState;
}
namespace exec
{
class Executor
{
public:
    explicit Executor( std::unique_ptr<shell::ShellState>& state, std::unique_ptr<builtin::BuiltinDispatcher>& builtins);

    int Run(const parser::AstNode& root);

private:
    int Exec(const parser::AstNode& n); // dynamic_cast dispatch
    int ExecList(const parser::ListNode&);
    int ExecAndOr(const parser::AndOrNode&);
    int ExecPipeline(const parser::PipelineNode&);
    int ExecSimple(const parser::SimpleCommand&);

    [[nodiscard]]
    CommandSpec BuildSpec(const parser::SimpleCommand&) const; // expand + env
    [[nodiscard]]
    std::vector<std::string> Expand(const std::vector<std::string>&) const;

    std::unique_ptr<shell::ShellState>& m_state_;
    std::unique_ptr<builtin::BuiltinDispatcher>& m_builtins_;
};
} // namespace exec
#endif // EXEC_EXECUTOR_HPP