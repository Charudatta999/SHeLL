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
namespace
{
    const int BUFFER_SIZE = 4096;
}

namespace exec
{
std::string CaptureOutput(
    const std::shared_ptr<parser::ast::AstNode>& root,
    std::unique_ptr<shell::ShellState>& state,
    std::unique_ptr<builtins::BuiltinDispatcher>& dispatcher,
    const shell::expander::CommandRunner& cmdRunner)
{
    auto pipe = io::Pipe();
    auto runner = ForkRunner();
    auto childFn = [&]() -> int
    {
        if(!io::fdops::Dup2(*pipe.GetWritePipeFD(), STDOUT_FILENO))
        {
            return 1;
        }
        pipe.CloseReadFD();
        pipe.CloseWriteFD();
        state->EnableJobControl(false);
        exec::Executor executor(state, dispatcher, cmdRunner,STDERR_FILENO );
        return executor.Run(root);
    };
    runner.Start(childFn);
    pipe.CloseWriteFD();

    std::string output;
    std::array<char, BUFFER_SIZE> buffer;
    ssize_t bytesRead = 0;
    int readFd = pipe.GetReadPipeFD()->GetFD();
    while ((bytesRead = read(readFd, buffer.data(), sizeof(buffer))) > 0)
    {
        output.append(buffer.data(), static_cast<std::size_t>(bytesRead));
    }
    pipe.CloseReadFD();

    // Read to EOF first, wait last — never the other way around.
    WaitStatus status(runner.Pid(),exec::WaitMode::UntilExit);

    while (!output.empty() && output.back() == '\n')
    {
        output.pop_back();
    }
    return output;
}
} // namespace exec