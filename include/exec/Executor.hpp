#ifndef EXEC_EXECUTOR_HPP
#define EXEC_EXECUTOR_HPP

#include "exec/ExecHelpers.hpp"
#include "parser/ast/AstVisitor.hpp"
#include "shell/expander/Expander.hpp"

#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

namespace builtins
{
class BuiltinDispatcher;
}

namespace shell
{
class ShellState;
}

namespace parser::ast
{
class AstNode;
}

namespace exec
{
struct PipelineResult;

class Executor : public parser::ast::AstVisitor
{
public:
    explicit Executor(
        std::unique_ptr<shell::ShellState>& state,
        std::unique_ptr<builtins ::BuiltinDispatcher>& builtins,
        const shell::expander::CommandRunner& cmdRunner,
        int outFd = STDOUT_FILENO);

    int Run(const std::unique_ptr<parser::ast::AstNode>& root);

private:
    void Visit(parser::ast::SimpleCommand&) override;
    void Visit(parser::ast::Pipeline&) override;
    void Visit(parser::ast::List&) override;
    void Visit(parser::ast::AndOr&) override;
    void Visit(parser::ast::Subshell&) override;
    void Visit(parser::ast::Group&) override;
    void Visit(parser::ast::Function&) override;
    void Visit(parser::ast::While&) override;
    void Visit(parser::ast::For&) override;
    void Visit(parser::ast::If&) override;
    void Visit(parser::ast::Case&) override;
    void Visit(parser::ast::ArithmeticCommand&) override;
    [[nodiscard]]
    CommandSpec BuildSpec(const std::vector<std::string>& argv,
                          const parser::ast::SimpleCommand&) const;
    [[nodiscard]]
    std::vector<std::string>
    ExpandArgv(const std::vector<std::string>& argv);
    void RecordStoppedJob(PipelineResult result,
                          const std::string& commandText);
    void Announce(const std::string& line) const;

    std::unique_ptr<shell::ShellState>& m_state_;
    std::unique_ptr<builtins ::BuiltinDispatcher>& m_builtins_;
    int m_status_ = 0;
    const shell::expander::CommandRunner& m_cmdRunner_;
    int m_outFd_;
    // True while traversing a compound (AndOr/List/loop/if/...). A stop
    // inside a compound can't be suspended as one job (the continuation
    // lives on the C++ stack), so RecordStoppedJob resumes-to-completion
    // instead of recording a misleading half-job.
    bool m_inCompound_ = false;
};
} // namespace exec
#endif // EXEC_EXECUTOR_HPP