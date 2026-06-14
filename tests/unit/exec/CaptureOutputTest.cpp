// =========================================================
// CaptureOutputTest — the real command-substitution capture:
// fork a child with stdout on a pipe, run the parsed command,
// read to EOF, strip trailing newlines. (ExpanderTest uses a
// fake runner; this exercises the actual fork+pipe path.)
// =========================================================

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>

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
