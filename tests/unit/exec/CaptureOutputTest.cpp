// =========================================================
// CaptureOutputTest — the real command-substitution capture:
// fork a child with stdout on a pipe, run the parsed command,
// read to EOF, strip trailing newlines. (ExpanderTest uses a
// fake runner; this exercises the actual fork+pipe path.)
// =========================================================

#include <gtest/gtest.h>

#include <csignal>
#include <map>
#include <memory>
#include <string>
#include <sys/time.h>

#include "builtins/BuiltinDispatcher.hpp"
#include "exec/CaptureOutput.hpp"
#include "parser/Parser.hpp"
#include "parser/Tokenizer.hpp"
#include "shell/ShellState.hpp"
#include "shell/expander/Expander.hpp"

namespace
{
std::string capture(const std::string& src)
{
    auto state = std::make_unique<shell::ShellState>(
        std::map<std::string, std::string>{});
    auto dispatcher =
        std::make_unique<builtins::BuiltinDispatcher>();
    shell::expander::CommandRunner runner =
        [](const std::string&) { return std::string{}; };

    parser::Tokenizer tokenizer(src);
    auto ast = parser::Parser(tokenizer.Tokenize()).Parse();
    return exec::CaptureOutput(ast, state, dispatcher, runner);
}
} // namespace

TEST(CaptureOutput, CapturesCommandStdout)
{
    EXPECT_EQ(capture("echo hello"), "hello");
}

TEST(CaptureOutput, StripsTrailingNewlines)
{
    // printf emits no trailing newline of its own here; force a couple.
    EXPECT_EQ(capture("printf 'x\\n\\n'"), "x");
}

TEST(CaptureOutput, CapturesMultipleCommands)
{
    EXPECT_EQ(capture("echo a; echo b"), "a\nb");
}

TEST(CaptureOutput, EmptyOutputIsEmptyString)
{
    EXPECT_EQ(capture("true"), "");
}

TEST(CaptureOutput, ProcessSubstitutionExpands)
{
    EXPECT_EQ(capture("cat <(printf hi)"), "hi");
}

// SIGCHLD is installed without SA_RESTART (signals/Sigchld.cpp), so the
// captured child's own exit can interrupt the capture read mid-stream.
// Treating that EINTR as EOF silently truncates `$(...)`. Reproduce it
// with a repeating SIGALRM while capturing far more than one buffer.
TEST(CaptureOutput, SignalDuringReadDoesNotTruncate)
{
    struct sigaction alarmAction
    {
    };
    alarmAction.sa_handler = [](int) {};
    alarmAction.sa_flags = 0; // no SA_RESTART: blocked reads get EINTR
    sigemptyset(&alarmAction.sa_mask);
    struct sigaction previous
    {
    };
    ASSERT_EQ(sigaction(SIGALRM, &alarmAction, &previous), 0);

    itimerval timer{};
    timer.it_value.tv_usec = 200;
    timer.it_interval.tv_usec = 200;
    ASSERT_EQ(setitimer(ITIMER_REAL, &timer, nullptr), 0);

    // ~108 KiB, so the loop makes many passes over the 4 KiB buffer.
    const std::string out = capture("seq 1 20000");

    const itimerval stop{};
    setitimer(ITIMER_REAL, &stop, nullptr);
    sigaction(SIGALRM, &previous, nullptr);

    ASSERT_FALSE(out.empty());
    EXPECT_EQ(out.substr(0, 2), "1\n");
    EXPECT_EQ(out.substr(out.size() - 5), "20000");
}
