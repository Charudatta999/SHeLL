#include "exec/CaptureOutput.hpp"

#include "builtins/BuiltinDispatcher.hpp"
#include "exec/Executor.hpp"
#include "parser/ast/AstNode.hpp"
#include "shell/ShellState.hpp"
#include "io/Pipe.hpp"
#include "exec/ForkRunner.hpp"
#include "exec/WaitStatus.hpp"
#include "io/FdOps.hpp"

#include <unistd.h>

namespace exec
{
std::string CaptureOutput(
    const std::unique_ptr<parser::ast::AstNode>& root,
    std::unique_ptr<shell::ShellState>& state,
    std::unique_ptr<builtins::BuiltinDispatcher>& dispatcher,
    const shell::expander::CommandRunner& cmdRunner)
{
    auto pipe = io::Pipe();
    auto runner = ForkRunner();
    auto childFn = [&]() -> int
    {
        if(!io::fdops::Dup2(*pipe.GetWritePipeFD(), 1))
        {
            return 1;
        }
        pipe.CloseReadFD();
        pipe.CloseWriteFD();
        exec::Executor executor(state, dispatcher, cmdRunner);
        return executor.Run(root);
    };
    runner.Start(childFn);
    pipe.CloseWriteFD();

    std::string output;
    char buffer[4096];
    ssize_t bytesRead = 0;
    int readFd = pipe.GetReadPipeFD()->GetFD();
    while ((bytesRead = read(readFd, buffer, sizeof(buffer))) > 0)
    {
        output.append(buffer, static_cast<std::size_t>(bytesRead));
    }
    pipe.CloseReadFD();

    // Read to EOF first, wait last — never the other way around.
    WaitStatus status(runner.Pid());

    while (!output.empty() && output.back() == '\n')
    {
        output.pop_back();
    }
    return output;
}
} // namespace exec