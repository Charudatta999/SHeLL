#ifndef SIGNALS_SIGNALMANAGER_HPP
#define SIGNALS_SIGNALMANAGER_HPP

#include <cstdint>
#include <optional>

namespace signals
{

class SignalManager
{
public:
    SignalManager() = default;
    ~SignalManager() = default;
    SignalManager(const SignalManager&) = delete;
    SignalManager& operator=(const SignalManager&) = delete;
    SignalManager(SignalManager&&) = delete;
    SignalManager& operator=(SignalManager&&) = delete;

    /// @brief Interactive shell setup (replaces the 4 inline calls in
    /// Run): ignore SIGTSTP/SIGTTIN/SIGTTOU/SIGINT/SIGQUIT + install
    /// SIGCHLD reaper
    void SetupInteractiveSignals();

    /// @brief Backs the future `trap` builtin:
    void SetHandler(int signo, void (*handler)(int));
    void Ignore(int signo);
    void Default(int signo);

private:
    void Apply(int signo, std::optional<void (*)(int)>handler);
};

} // namespace signals
#endif // SIGNALS_SIGNALMANAGER_HPP