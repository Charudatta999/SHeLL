// =========================================================
// SelectTest — the `select` loop: menu on stderr, PS3 prompt,
// reply parsing (valid / invalid / empty / EOF) and break
// integration. stdin is swapped for a pipe pre-loaded with the
// simulated user input; stderr is captured through a pipe too.
// =========================================================

#include <gtest/gtest.h>

#include <fcntl.h>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>

#include "builtins/BuiltinDispatcher.hpp"
#include "exec/Executor.hpp"
#include "parser/Parser.hpp"
#include "parser/Tokenizer.hpp"
#include "shell/ShellState.hpp"

namespace
{
struct RunResult
{
    int status;
    std::string err;
    std::string out;
};

// Runs `src` with `input` as stdin and returns the exit status plus
// everything the shell wrote to stderr (menu + prompts) and stdout.
RunResult runWithInput(const std::string& src,
                       const std::string& input)
{
    int inPipe[2];
    int errPipe[2];
    int outPipe[2];
    EXPECT_EQ(pipe(inPipe), 0);
    EXPECT_EQ(pipe(errPipe), 0);
    EXPECT_EQ(pipe(outPipe), 0);
    EXPECT_EQ(write(inPipe[1], input.data(), input.size()),
              static_cast<ssize_t>(input.size()));
    close(inPipe[1]); // reader sees EOF after the scripted input

    fflush(stdout);
    const int savedIn = dup(STDIN_FILENO);
    const int savedErr = dup(STDERR_FILENO);
    const int savedOut = dup(STDOUT_FILENO);
    dup2(inPipe[0], STDIN_FILENO);
    dup2(errPipe[1], STDERR_FILENO);
    dup2(outPipe[1], STDOUT_FILENO);
    close(inPipe[0]);
    close(errPipe[1]);
    close(outPipe[1]);

    auto state = std::make_unique<shell::ShellState>(
        std::map<std::string, std::string>{});
    auto disp = std::make_unique<builtins::BuiltinDispatcher>();
    shell::expander::CommandRunner stubRunner =
        [](const std::string&) { return std::string{}; };
    exec::Executor exec(state, disp, stubRunner);

    parser::Tokenizer tok(src);
    parser::Parser    p(tok.Tokenize());
    auto              tree = p.Parse();
    const int         status = exec.Run(tree);

    fflush(stdout);
    dup2(savedIn, STDIN_FILENO);
    dup2(savedErr, STDERR_FILENO);
    dup2(savedOut, STDOUT_FILENO);
    close(savedIn);
    close(savedErr);
    close(savedOut);

    const auto drain = [](int fd)
    {
        std::string data;
        char        buf[4096];
        ssize_t     n = 0;
        while ((n = read(fd, buf, sizeof buf)) > 0)
            data.append(buf, static_cast<std::size_t>(n));
        close(fd);
        return data;
    };

    return {.status = status,
            .err = drain(errPipe[0]),
            .out = drain(outPipe[0])};
}
} // namespace

TEST(Select, BindsChosenWordAndBreaks)
{
    auto res = runWithInput(
        "select x in 10 20 30; do break; done; ((x == 20))", "2\n");
    EXPECT_EQ(res.status, 0);
}

TEST(Select, PrintsMenuAndPromptOnStderr)
{
    auto res = runWithInput("select x in a b; do break; done", "1\n");
    EXPECT_NE(res.err.find("1) a\n"), std::string::npos);
    EXPECT_NE(res.err.find("2) b\n"), std::string::npos);
    EXPECT_NE(res.err.find("#? "), std::string::npos);
}

TEST(Select, UsesPs3AsPrompt)
{
    // Quoted assignment values are not supported yet, so keep the
    // prompt a single unquoted word.
    auto res = runWithInput(
        "PS3=pick:; select x in a; do break; done", "1\n");
    EXPECT_NE(res.err.find("pick:"), std::string::npos);
    EXPECT_EQ(res.err.find("#? "), std::string::npos);
}

TEST(Select, OutOfRangeChoiceBindsEmpty)
{
    auto res = runWithInput(
        "select x in 10 20; do break; done; ((x == 0))", "99\n");
    EXPECT_EQ(res.status, 0);
}

TEST(Select, NonNumericChoiceBindsEmptyButSetsReply)
{
    auto res = runWithInput("select x in 10 20; do break; done; "
                            "((x == 0)) && echo $REPLY",
                            "zz\n");
    EXPECT_EQ(res.status, 0);
    EXPECT_EQ(res.out, "zz\n");
}

TEST(Select, ReplyHoldsRawInputOnValidChoice)
{
    auto res = runWithInput(
        "select x in a b c; do break; done; ((REPLY == 3))", "3\n");
    EXPECT_EQ(res.status, 0);
}

TEST(Select, EmptyLineRedisplaysMenuWithoutRunningBody)
{
    auto res = runWithInput("n=0; select x in a; do ((n++)); break; "
                            "done; ((n == 1))",
                            "\n1\n");
    EXPECT_EQ(res.status, 0);
    // Menu printed twice: once initially, once after the bare Enter.
    const auto first = res.err.find("1) a\n");
    ASSERT_NE(first, std::string::npos);
    EXPECT_NE(res.err.find("1) a\n", first + 1), std::string::npos);
}

TEST(Select, InvalidChoiceLoopsWithoutReprintingMenu)
{
    auto res = runWithInput("n=0; select x in a; do ((n++)); "
                            "if ((n == 2)); then break; fi; done; "
                            "((n == 2))",
                            "9\n1\n");
    EXPECT_EQ(res.status, 0);
    // Only the initial menu; invalid input reprompts without it.
    const auto first = res.err.find("1) a\n");
    ASSERT_NE(first, std::string::npos);
    EXPECT_EQ(res.err.find("1) a\n", first + 1), std::string::npos);
}

TEST(Select, EofEndsLoopWithoutRunningBody)
{
    auto res = runWithInput(
        "n=0; select x in a b; do ((n++)); done; ((n == 0))", "");
    EXPECT_EQ(res.status, 0);
}

TEST(Select, EmptyWordListRunsNothing)
{
    auto res = runWithInput("select x in; do false; done", "1\n");
    EXPECT_EQ(res.status, 0);
    EXPECT_TRUE(res.err.empty());
}

TEST(Select, ContinueReprompts)
{
    auto res = runWithInput("n=0; select x in a; do ((n++)); "
                            "if ((n < 2)); then continue; fi; break; "
                            "done; ((n == 2))",
                            "1\n1\n");
    EXPECT_EQ(res.status, 0);
}

TEST(Select, BreakTwoExitsEnclosingLoop)
{
    auto res = runWithInput("n=0; for i in 1 2; do ((n++)); "
                            "select x in a; do break 2; done; "
                            "((n = n + 100)); done; ((n == 1))",
                            "1\n");
    EXPECT_EQ(res.status, 0);
}

TEST(Select, ExpandsMenuWords)
{
    auto res = runWithInput("v=20; select x in 10 $v; do break; "
                            "done; ((x == 20))",
                            "2\n");
    EXPECT_EQ(res.status, 0);
    EXPECT_NE(res.err.find("2) 20\n"), std::string::npos);
}
