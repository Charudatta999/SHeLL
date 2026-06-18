#ifndef EXEC_EXECUTOR_HPP
#define EXEC_EXECUTOR_HPP

#include "exec/ExecHelpers.hpp"
#include "exec/SuspendedCoro.hpp"

#include "coro/Task.hpp"
#include "parser/ast/ExecVisitor.hpp"
#include "shell/expander/Expander.hpp"

#include <coroutine>
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

class Executor : public parser::ast::ExecVisitor
{
public:
    explicit Executor(
        std::unique_ptr<shell::ShellState>& state,
        std::unique_ptr<builtins ::BuiltinDispatcher>& builtins,
        const shell::expander::CommandRunner& cmdRunner,
        int outFd = STDOUT_FILENO);

    int Run(const std::shared_ptr<parser::ast::AstNode>& root);

private:
    coro::Task Visit(parser::ast::SimpleCommand&) override;
    coro::Task Visit(parser::ast::Pipeline&) override;
    coro::Task Visit(parser::ast::List&) override;
    coro::Task Visit(parser::ast::AndOr&) override;
    coro::Task Visit(parser::ast::Subshell&) override;
    coro::Task Visit(parser::ast::Group&) override;
    coro::Task Visit(parser::ast::Function&) override;
    coro::Task Visit(parser::ast::While&) override;
    coro::Task Visit(parser::ast::For&) override;
    coro::Task Visit(parser::ast::If&) override;
    coro::Task Visit(parser::ast::Case&) override;
    coro::Task Visit(parser::ast::ArithmeticCommand&) override;

    [[nodiscard]]
    CommandSpec BuildSpec(const std::vector<std::string>& argv,
                          const parser::ast::SimpleCommand&) const;
    [[nodiscard]]
    std::vector<std::string>
    ExpandArgv(const std::vector<std::string>& argv);
    void RecordStoppedJob(PipelineResult result,
                          const std::string& commandText);
    void Announce(const std::string& line) const;

    int RunToCompletion(const std::unique_ptr<parser::ast::AstNode>& node);

    // Thaw a frozen compound: feed the resumed leaf's real status in, wake
    // the suspended frame, and drive the rest to completion or the next
    // freeze. Called (via the captured callable) from fg; the Executor owns
    // the resume logic, fg only supplies the leaf status.
    SuspendedCoro::ResumeResult ResumeSuspended(SuspendedCoro& rec,
                                                int leafStatus);

    std::unique_ptr<shell::ShellState>& m_state_;
    std::unique_ptr<builtins ::BuiltinDispatcher>& m_builtins_;
    int m_status_ = 0;
    const shell::expander::CommandRunner& m_cmdRunner_;
    int m_outFd_;
    bool m_inCompound_ = false;
    std::coroutine_handle<> m_suspendedHandle_;
    pid_t m_suspendedPgid_;
    int m_resumedStatus_;
};
} // namespace exec
#endif // EXEC_EXECUTOR_HPP