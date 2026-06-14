#ifndef SIGNALS_SIGCHLD_HPP
#define SIGNALS_SIGCHLD_HPP

#include <atomic>
namespace signals
{

class Sigchld
{
public:
    Sigchld() = delete;
    ~Sigchld() = delete;
    Sigchld(const Sigchld&) = delete;
    Sigchld& operator=(const Sigchld&) = delete;
    Sigchld(Sigchld&&) = delete;
    Sigchld& operator=(Sigchld&&) = delete;

    static void Install();
    [[nodiscard]]
    static bool Consume();

private:
    static void Handler(int signum);
    static std::atomic<bool> s_childEvent_;
};

} // namespace signals
#endif // SIGNALS_SIGCHLD_HPP