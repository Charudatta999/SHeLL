#include "exec/CaptureOutput.hpp"

#include "builtins/BuiltinDispatcher.hpp"
#include "exec/ExecException.hpp"
#include "exec/Executor.hpp"
#include "exec/ProcessExecutor.hpp"
#include "exec/ProcessSubstitution.hpp"
#include "exec/WaitStatus.hpp"
#include "io/FdOps.hpp"
#include "io/Pipe.hpp"
#include "parser/ast/AstNode.hpp"
#include "shell/ShellState.hpp"

#include <array>
#include <cerrno>
#include <exception>
#include <string>
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
    const shell::expander::CommandRunner& cmdRunner,
    const shell::expander::ProcSubRunner& procSubRunner)
{
    auto pipe = io::Pipe();
    auto runner = ProcessExecutor();
    auto childFn = [&]() -> int
    {
        // Same rule as StartProcessSub: never let an exception escape
        // the forked child back into a copied REPL stack.
        try
        {
            if (!io::fdops::Dup2(*pipe.GetWritePipeFD(),
                                 STDOUT_FILENO))
                return 1;
            pipe.CloseReadFD();
            pipe.CloseWriteFD();
            state->EnableJobControl(false);
            shell::expander::ProcSubRunner nested = procSubRunner;
            if (!nested)
                nested =
                    MakeProcSubRunner(state, dispatcher, cmdRunner);
            exec::Executor executor(state,
                                    dispatcher,
                                    cmdRunner,
                                    STDERR_FILENO,
                                    nested);
            return executor.Run(root);
        }
        catch (const std::exception& ex)
        {
            std::string msg = std::string(ex.what()) + "\n";
            io::fdops::WriteAll(STDERR_FILENO, msg);
            return 1;
        }
        catch (...)
        {
            io::fdops::WriteAll(STDERR_FILENO,
                                "command substitution: unknown error\n");
            return 1;
        }
    };
    runner.Fork(childFn, 0);
    if (runner.Pid() < 0)
    {
        throw ExecException("command substitution: fork failed",
                            errno);
    }
    pipe.CloseWriteFD();

    std::string output;
    std::array<char, BUFFER_SIZE> buffer;
    int readFd = pipe.GetReadPipeFD()->GetFD();
    while (true)
    {
        const ssize_t bytesRead =
            read(readFd, buffer.data(), buffer.size());
        if (bytesRead > 0)
        {
            output.append(buffer.data(),
                          static_cast<std::size_t>(bytesRead));
            continue;
        }
        if (bytesRead == 0)
            break; // EOF: the child closed the write end.
        // SIGCHLD is installed without SA_RESTART (signals/Sigchld.cpp),
        // so the captured child's own exit can interrupt this read.
        // Treating EINTR as EOF would silently truncate `$(...)`.
        if (errno == EINTR)
            continue;
        break;
    }
    pipe.CloseReadFD();

    // Read to EOF first, wait last — never the other way around.
    WaitStatus status(runner.Pid(), exec::WaitMode::UntilExit);

    while (!output.empty() && output.back() == '\n')
        output.pop_back();
    return output;
}
} // namespace exec
