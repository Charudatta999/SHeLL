// =========================================================
// SuspendBuiltinTest — #48: `suspend` stops the shell itself.
//
// The guard paths return normally and are safe to run in-process.
// The success path genuinely stops whoever runs it, so that case is
// exercised in a forked child placed in its own process group — the
// builtin signals its whole group, and without that isolation it
// would stop the test runner too.
// =========================================================

#include <gtest/gtest.h>

#include <cerrno>
#include <csignal>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include "builtins/BuiltinDispatcher.hpp"
#include "exec/Executor.hpp"
#include "parser/Parser.hpp"
#include "parser/Tokenizer.hpp"
#include "shell/Repl.hpp"
#include "shell/ShellState.hpp"

namespace
{
std::unique_ptr<shell::ShellState> freshState()
{
    return std::make_unique<shell::ShellState>(
        std::map<std::string, std::string>{});
}

int runOn(std::unique_ptr<shell::ShellState>& state,
          const std::string& src)
{
    auto disp = std::make_unique<builtins::BuiltinDispatcher>();
    shell::expander::CommandRunner stubRunner =
        [](const std::string&) { return std::string{}; };
    exec::Executor exec(state, disp, stubRunner);

    parser::Tokenizer tok(src);
    parser::Parser parse(tok.Tokenize());
    auto tree = parse.Parse();
    return exec.Run(tree);
}

// Sibling suites install a SIGCHLD handler without SA_RESTART (see
// signals/Sigchld.cpp), so a blocking wait here can be interrupted by
// an unrelated child exiting. Retry like WaitStatus does.
pid_t WaitEintr(pid_t pid, int& status, int flags)
{
    pid_t res = -1;
    while ((res = waitpid(pid, &status, flags)) == -1 && errno == EINTR)
    {
    }
    return res;
}

// Runs `src` in a forked child that owns its process group, and
// asserts the child stops on SIGTSTP, then resumes and exits cleanly.
void ExpectSuspends(const std::function<void(
                        std::unique_ptr<shell::ShellState>&)>& setup,
                    const std::string& src)
{
    pid_t pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0)
    {
        // Own process group: the builtin signals its whole group, and
        // the test runner must not be caught in it.
        setpgid(0, 0);
        auto state = freshState();
        state->EnableJobControl(true);
        setup(state);
        runOn(state, src);
        _exit(0);
    }

    // Poll rather than block, so a regression fails the test instead
    // of hanging the suite forever.
    int status = 0;
    bool stopped = false;
    for (int i = 0; i < 300 && !stopped; ++i)
    {
        const pid_t res = WaitEintr(pid, status, WUNTRACED | WNOHANG);
        if (res == pid && WIFSTOPPED(status))
            stopped = true;
        else
            usleep(10000); // 10ms
    }

    if (!stopped)
    {
        kill(pid, SIGKILL);
        WaitEintr(pid, status, 0);
        FAIL() << "`" << src << "` did not stop the process";
    }
    EXPECT_EQ(WSTOPSIG(status), SIGTSTP);

    // Resuming must let the builtin return and the child exit cleanly.
    ASSERT_EQ(kill(pid, SIGCONT), 0);
    ASSERT_EQ(WaitEintr(pid, status, 0), pid);
    EXPECT_TRUE(WIFEXITED(status));
    EXPECT_EQ(WEXITSTATUS(status), 0);
}
} // namespace

TEST(SuspendBuiltin, RefusesWithoutJobControl)
{
    // Nothing would be left to send SIGCONT, so this must not stop.
    auto state = freshState();
    ASSERT_FALSE(state->IsJobControlEnabled());
    EXPECT_EQ(runOn(state, "suspend"), 1);
}

TEST(SuspendBuiltin, ForceDoesNotOverrideMissingJobControl)
{
    auto state = freshState();
    EXPECT_EQ(runOn(state, "suspend --force"), 1);
}

TEST(SuspendBuiltin, RefusesLoginShellWithoutForce)
{
    auto state = freshState();
    state->EnableJobControl(true);
    state->SetLoginShell(true);
    EXPECT_EQ(runOn(state, "suspend"), 1);
}

TEST(SuspendBuiltin, RejectsUnknownOption)
{
    auto state = freshState();
    state->EnableJobControl(true);
    EXPECT_EQ(runOn(state, "suspend -x"), 2);
}

TEST(SuspendBuiltin, DetectsLoginInvocation)
{
    // login(1)/sshd spell a login shell as a leading '-' on argv[0].
    const char* dashArgv0[] = {"-shellrepl"};
    EXPECT_TRUE(shell::Repl::IsLoginInvocation(1, dashArgv0));

    const char* plain[] = {"shellrepl"};
    EXPECT_FALSE(shell::Repl::IsLoginInvocation(1, plain));

    const char* longFlag[] = {"shellrepl", "--login"};
    EXPECT_TRUE(shell::Repl::IsLoginInvocation(2, longFlag));

    const char* shortFlag[] = {"shellrepl", "-l"};
    EXPECT_TRUE(shell::Repl::IsLoginInvocation(2, shortFlag));

    const char* unrelated[] = {"shellrepl", "-x", "script.sh"};
    EXPECT_FALSE(shell::Repl::IsLoginInvocation(3, unrelated));

    EXPECT_FALSE(shell::Repl::IsLoginInvocation(0, nullptr));
}

TEST(SuspendBuiltin, StopsTheProcessThenResumes)
{
    ExpectSuspends([](std::unique_ptr<shell::ShellState>&) {},
                   "suspend");
}

TEST(SuspendBuiltin, ForceStopsALoginShell)
{
    ExpectSuspends([](std::unique_ptr<shell::ShellState>& state)
                   { state->SetLoginShell(true); },
                   "suspend --force");
}