#include "signals/SignalManager.hpp"

#include "signals/Sigchld.hpp"

#include <csignal>
#include <optional>

namespace signals
{
void SignalManager::SetupInteractiveSignals()
{
    Ignore(SIGTSTP);
    Ignore(SIGTTIN);
    Ignore(SIGTTOU);
    Ignore(SIGINT);
    Ignore(SIGQUIT);
    Sigchld::Install();
}

void SignalManager::SetHandler(int signo, void (*handler)(int))
{
    Apply(signo, handler);
}

void SignalManager::Ignore(int signo)
{
    Apply(signo, SIG_IGN);
}

void SignalManager::Default(int signo)
{
    Apply(signo, SIG_DFL);
}

void SignalManager::Apply(int signo,
                          std::optional<void (*)(int)> handler)
{
    struct sigaction action{};
    action.sa_handler = handler.value_or(SIG_DFL);
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(signo, &action, nullptr);
}

void SignalManager::ResetForChild()
{
    Default(SIGTSTP);
    Default(SIGTTIN);
    Default(SIGTTOU);
    Default(SIGINT);
    Default(SIGQUIT);
}
} // namespace signals