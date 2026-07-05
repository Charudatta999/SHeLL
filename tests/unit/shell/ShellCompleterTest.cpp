// =========================================================
// ShellCompleterTest — unit tests for shell::ShellCompleter.
// Tests: word scanning, command vs argument classification,
// file completion against real directories, edge cases.
// PATH is never set on the test ShellState, so command
// completion is deterministic: builtins only.
// =========================================================

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib> // IWYU pragma: keep (mkdtemp)
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include "builtins/BuiltinDispatcher.hpp"
#include "line/Completer.hpp"
#include "shell/ShellCompleter.hpp"
#include "shell/ShellState.hpp"

namespace fs = std::filesystem;

namespace
{
/// @brief Make a ShellState for testing with optional initial variables.
std::unique_ptr<shell::ShellState> MakeState(
    const std::map<std::string, std::string>& vars = {})
{
    return std::make_unique<shell::ShellState>(vars);
}

/// @brief Make a BuiltinDispatcher for testing.
std::unique_ptr<builtins::BuiltinDispatcher> MakeDispatcher()
{
    return std::make_unique<builtins::BuiltinDispatcher>();
}

/// @brief RAII temp directory: created on construction, removed with all
/// its contents on destruction (so a failing test cannot leak it).
struct TempDir
{
    TempDir()
    {
        std::string tmpl = "/tmp/completion_test_XXXXXX";
        if (mkdtemp(tmpl.data()) == nullptr)
            throw std::runtime_error("Failed to create temp directory");
        path = tmpl;
    }
    ~TempDir()
    {
        std::error_code errorCode; // non-throwing overload: dtor stays noexcept
        fs::remove_all(path, errorCode);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    TempDir(TempDir&&) = delete;
    TempDir& operator=(TempDir&&) = delete;

    /// @brief Create an empty regular file inside the directory.
    void Touch(const std::string& name) const
    {
        const std::ofstream file(path + "/" + name);
    }

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes): test helper
    std::string path;
};

/// @brief The candidate texts of a result, in result order.
std::vector<std::string> Texts(const line::Result& result)
{
    std::vector<std::string> texts;
    texts.reserve(result.candidates.size());
    for (const auto& candidate : result.candidates)
        texts.push_back(candidate.text);
    return texts;
}

} // namespace

// ─── Word Scan and replaceStart ──────────────────────────────────────────────

TEST(ShellCompleter, WordScanStopsAtSpace)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // "ls -l /tm" at cursor 9 (end of line): word "/tm" starts at index 6,
    // and a path word replaces past its last slash (index 7).
    const line::Result result = completer.Complete("ls -l /tm", 9);
    EXPECT_EQ(result.replaceStart, 7);
}

TEST(ShellCompleter, WordScanAtCursorMidWord)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // Cursor 8: word is "/t" (only what is left of the cursor counts).
    const line::Result result = completer.Complete("ls -l /tm", 8);
    EXPECT_EQ(result.replaceStart, 7);
}

TEST(ShellCompleter, WordStartsAtBeginningOfLine)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    const line::Result result = completer.Complete("ls", 2);
    EXPECT_EQ(result.replaceStart, 0);
}

// ─── Command vs Argument Classification ──────────────────────────────────────

TEST(ShellCompleter, CommandAtStartOfLine)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // No PATH on the state, so "e" can only match builtins: echo, exit.
    const line::Result result = completer.Complete("e", 1);
    EXPECT_EQ(Texts(result), (std::vector<std::string>{"echo", "exit"}));
    EXPECT_EQ(result.replaceStart, 0);
}

TEST(ShellCompleter, CommandCandidatesCarryDescriptions)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    const line::Result result = completer.Complete("cd", 2);
    ASSERT_EQ(result.candidates.size(), 1);
    EXPECT_EQ(result.candidates.front().text, "cd");
    EXPECT_EQ(result.candidates.front().description,
              dispatcher->Description("cd"));
}

TEST(ShellCompleter, ArgumentAfterCommand)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // "ls fi" — "fi" is an argument (a file), replaced from index 3.
    const line::Result result = completer.Complete("ls fi", 5);
    EXPECT_EQ(result.replaceStart, 3);
}

TEST(ShellCompleter, CommandAfterPipelineOperator)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // "cat f | ec" — "ec" sits after '|', so it is a command again.
    const line::Result result = completer.Complete("cat f | ec", 10);
    EXPECT_EQ(result.replaceStart, 8);
    EXPECT_EQ(Texts(result), (std::vector<std::string>{"echo"}));
}

TEST(ShellCompleter, CommandAfterSemicolonOperator)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    const line::Result result = completer.Complete("ls ; ec", 7);
    EXPECT_EQ(result.replaceStart, 5);
    EXPECT_EQ(Texts(result), (std::vector<std::string>{"echo"}));
}

TEST(ShellCompleter, CommandAfterAmpersandOperator)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    const line::Result result = completer.Complete("ls & ec", 7);
    EXPECT_EQ(result.replaceStart, 5);
    EXPECT_EQ(Texts(result), (std::vector<std::string>{"echo"}));
}

// ─── File Completion against Real Temp Directory ─────────────────────────────

TEST(ShellCompleter, FileCompletionCandidates)
{
    const TempDir tmp;
    tmp.Touch("alpha.txt");
    tmp.Touch("alps");
    fs::create_directory(tmp.path + "/arch");

    auto state = MakeState();
    state->SetCWD(tmp.path);
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // Exactly "alpha.txt" and "alps" start with "al", already sorted.
    const line::Result result = completer.Complete("ls al", 5);
    EXPECT_EQ(Texts(result), (std::vector<std::string>{"alpha.txt", "alps"}));
}

TEST(ShellCompleter, DirectoryCandidateGetsTrailingSlash)
{
    const TempDir tmp;
    fs::create_directory(tmp.path + "/subdir");

    auto state = MakeState();
    state->SetCWD(tmp.path);
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    const line::Result result = completer.Complete("ls sub", 6);
    ASSERT_EQ(result.candidates.size(), 1);
    EXPECT_EQ(result.candidates.front().text, "subdir/");
    EXPECT_EQ(result.candidates.front().description, "directory");
}

TEST(ShellCompleter, HiddenFilesSkippedUnlessBaseStartsWithDot)
{
    const TempDir tmp;
    tmp.Touch("visible");
    tmp.Touch(".hidden");

    auto state = MakeState();
    state->SetCWD(tmp.path);
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // Empty base hides dotfiles: only "visible" shows up.
    const line::Result all = completer.Complete("ls ", 3);
    EXPECT_EQ(Texts(all), (std::vector<std::string>{"visible"}));

    // A base starting with '.' reveals them, and only them.
    const line::Result dotted = completer.Complete("ls .", 4);
    EXPECT_EQ(Texts(dotted), (std::vector<std::string>{".hidden"}));
}

TEST(ShellCompleter, PathWordSplitOnLastSlash)
{
    const TempDir tmp;
    fs::create_directory(tmp.path + "/subdir");
    tmp.Touch("subdir/fileA.txt");
    tmp.Touch("subdir/fileB");

    auto state = MakeState();
    state->SetCWD(tmp.path);
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // Word "subdir/file" starts at 3, '/' at 9: candidates are basenames
    // from subdir/ and replace from index 10 (past the slash).
    const std::string input = "ls subdir/file";
    const line::Result result = completer.Complete(input, input.size());
    EXPECT_EQ(result.replaceStart, 10);
    EXPECT_EQ(Texts(result), (std::vector<std::string>{"fileA.txt", "fileB"}));
}

// ─── Malformed and Edge Input ───────────────────────────────────────────────

TEST(ShellCompleter, EmptyLineOffersAllBuiltins)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // Empty word at line start matches every builtin (PATH is unset).
    const line::Result result = completer.Complete("", 0);
    auto names = dispatcher->Names();
    std::ranges::sort(names);
    EXPECT_EQ(Texts(result), names);
}

TEST(ShellCompleter, CursorClamped)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // Cursor far past line.size() must clamp, not crash or misplace.
    const line::Result result = completer.Complete("ls", 100);
    EXPECT_EQ(result.replaceStart, 0);
}

TEST(ShellCompleter, CursorAtStartOfLine)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // Cursor 0 in front of existing text: empty word, command position.
    const line::Result result = completer.Complete("abc def", 0);
    EXPECT_EQ(result.replaceStart, 0);
}

TEST(ShellCompleter, WordThatIsOnlySlash)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    // Word "/": dirPath "/", empty base. Root always has entries.
    const line::Result result = completer.Complete("ls /", 4);
    EXPECT_EQ(result.replaceStart, 4);
    EXPECT_FALSE(result.candidates.empty());
}

TEST(ShellCompleter, CandidatesSorted)
{
    auto state = MakeState();
    auto dispatcher = MakeDispatcher();
    const shell::ShellCompleter completer(state, dispatcher);

    const line::Result result = completer.Complete("", 0);
    EXPECT_TRUE(std::ranges::is_sorted(Texts(result)));
}
