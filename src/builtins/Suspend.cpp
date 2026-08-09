#include "builtins/BuiltInFunction.hpp"

#include "io/FdOps.hpp"

#include <csignal>
#include <string>
#include <unistd.h>
#include <vector>

namespace builtins
{
int Suspend(const std::vector<std::string>& argv,
            std::unique_ptr<BuiltinContext>& ctx)
{
    bool force = false;
    for (std::size_t i = 1; i < argv.size(); ++i)
    {
        if (argv[i] == "-f" || argv[i] == "--force")
        {
            force = true;
            continue;
        }
        io::fdops::WriteAll(ctx->errFd,
                            "suspend: " + argv[i] +
                                ": invalid option\n");
        return 2;
    }

    // Without job control there is no parent monitor shell to resume
    // us, so stopping here would strand the shell unrecoverably (a
    // piped or scripted `suspend` would simply hang forever). bash
    // refuses in exactly this case, and --force does not override it.
    if (!ctx->m_state_->IsJobControlEnabled())
    {
        io::fdops::WriteAll(ctx->errFd, "suspend: cannot suspend\n");
        return 1;
    }

    // Stopping a login shell drops the user out of their session with
    // nothing left to type into, so it takes an explicit --force.
    if (ctx->m_state_->IsLoginShell() && !force)
    {
        io::fdops::WriteAll(ctx->errFd,
                            "suspend: cannot suspend a login shell\n");
        return 1;
    }

    // An interactive shell ignores SIGTSTP (SignalManager::
    // SetupInteractiveSignals) so that Ctrl-Z stops the foreground job
    // rather than the shell. Restore the default disposition just long
    // enough for this one signal to land, then put the old handler
    // back once we are resumed.
    struct sigaction defaultAction{};
    defaultAction.sa_handler = SIG_DFL;
    sigemptyset(&defaultAction.sa_mask);
    defaultAction.sa_flags = 0;

    struct sigaction priorAction{};
    sigaction(SIGTSTP, &defaultAction, &priorAction);

    // Signal the whole process group, like bash's killpg(shell_pgrp):
    // with job control on the children live in their own groups, so
    // this reaches the shell alone. Delivery is synchronous, so this
    // call does not return until a SIGCONT arrives.
    kill(0, SIGTSTP);

    sigaction(SIGTSTP, &priorAction, nullptr);

    // Resumed: the terminal changed hands while we were stopped.
    ctx->m_state_->RestoreTerminalModes();
    return 0;
}
} // namespace builtins