#include "exec/ProcessSubstitution.hpp"

#include "builtins/BuiltinDispatcher.hpp"
#include "exec/ExecException.hpp"
#include "exec/Executor.hpp"
#include "exec/ProcessExecutor.hpp"
#include "io/FdOps.hpp"
#include "io/Pipe.hpp"
#include "parser/ast/AstNode.hpp"
#include "shell/ShellState.hpp"
#include "utils/ErrorCodes.hpp"

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

namespace exec
{
namespace
{
// psChild (the process forked below) never execs anything itself —
// it just drives the Executor coroutine tree in-process and forks a
// further child for the substitution body's external command. That
// means O_CLOEXEC never fires for *it*, so a plain fork() hands it
// verbatim copies of every fd its parent had open, including sibling
// pipeline pipes (e.g. the pipe between `echo` and `tee` in
// `echo hi | tee >(cmd)`) that were only ever meant to be closed by
// that pipeline stage's own exec(). A stray copy left open here keeps
// such a pipe's write end alive, so the real reader never sees EOF —
// a real deadlock reproduced by `echo hi | tee >(cat > f)`. Closing
// every inherited close-on-exec fd here simulates the exec() this
// process will never perform, matching what would have happened had
// the substitution's body run as a normal exec'd child.
void CloseInheritedCloexecFds()
{
    long maxFd = sysconf(_SC_OPEN_MAX);
    if (maxFd <= 0)
        maxFd = 1024;
    for (int fdesc = 3; fdesc < maxFd; ++fdesc)
    {
        int flags = fcntl(fdesc, F_GETFD);
        if (flags != -1 && (flags & FD_CLOEXEC))
            close(fdesc);
    }
}
} // namespace

std::string StartProcessSub(
    const std::shared_ptr<parser::ast::AstNode>& root,
    std::unique_ptr<shell::ShellState>& state,
    std::unique_ptr<builtins::BuiltinDispatcher>& dispatcher,
    const shell::expander::CommandRunner& cmdRunner,
    bool writeMode)
{
    auto pipe = io::Pipe();
    auto runner = ProcessExecutor();
    auto childFn = [&]() -> int
    {
        // <(cmd): child writes to the pipe (stdout). >(cmd): child
        // reads from the pipe (stdin).
        const auto& childEndFd = writeMode ? pipe.GetReadPipeFD()
                                           : pipe.GetWritePipeFD();
        const int targetFd = writeMode ? STDIN_FILENO : STDOUT_FILENO;
        if (!io::fdops::Dup2(*childEndFd, targetFd))
        {
            return 1;
        }
        pipe.CloseReadFD();
        pipe.CloseWriteFD();
        CloseInheritedCloexecFds();
        state->EnableJobControl(false);
        Executor executor(state, dispatcher, cmdRunner, STDERR_FILENO);
        return executor.Run(root);
    };
    runner.Fork(childFn, 0);

    // The parent keeps the other end: the read end for <(cmd), the
    // write end for >(cmd).
    if (writeMode)
    {
        pipe.CloseReadFD();
    }
    else
    {
        pipe.CloseWriteFD();
    }
    const int keptFd = writeMode ? pipe.GetWritePipeFD()->GetFD()
                                 : pipe.GetReadPipeFD()->GetFD();

    // io::Pipe fds are O_CLOEXEC (by design, so pipeline fds don't
    // leak into unrelated children); but /dev/fd/N has to survive the
    // outer command's exec to be openable there. Duplicate onto an
    // inheritable fd; the original (O_CLOEXEC) fd closes when `pipe`
    // goes out of scope below.
    const int inheritableFd = fcntl(keptFd, F_DUPFD, 10);
    if (inheritableFd < 0)
    {
        throw ExecException(
            "process substitution: failed to duplicate fd", errno);
    }

    state->AddProcSub(inheritableFd, runner.Pid());
    return "/dev/fd/" + std::to_string(inheritableFd);
}
} // namespace exec
