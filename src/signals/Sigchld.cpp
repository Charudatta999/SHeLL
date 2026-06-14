#include "signals/Sigchld.hpp"

#include <csignal>
#include <cstring>
namespace signals
{
std::atomic<bool> Sigchld::s_childEvent_{false};

void Sigchld::Install()
{
    struct sigaction action{};            // zero-initialised config struct
    action.sa_handler = Handler;          // function the kernel calls on SIGCHLD
    sigemptyset(&action.sa_mask);         // don't block other signals during the handler
    action.sa_flags = 0;                  // no SA_RESTART -> blocked reads get EINTR
    sigaction(SIGCHLD, &action, nullptr); // register it
}
bool Sigchld::Consume()
{
    return s_childEvent_.exchange(false, std::memory_order_relaxed);
}
void Sigchld::Handler(int /*signum*/)
{
    s_childEvent_.store(true, std::memory_order_relaxed);
}
}