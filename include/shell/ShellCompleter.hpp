#ifndef SHELL_SHELL_COMPLETER_HPP
#define SHELL_SHELL_COMPLETER_HPP

#include "builtins/BuiltinDispatcher.hpp"
#include "line/Completer.hpp"
#include "shell/ShellState.hpp"

#include <memory>

namespace shell
{

/// @brief Completer over the live shell: builtins, PATH executables,
/// and paths.
/// @details Implements line::Completer. In command position it offers
/// builtin names (with descriptions) plus PATH executables; in
/// argument position it offers filesystem entries. Reads PATH/cwd
/// from ShellState and builtin names from BuiltinDispatcher, both
/// borrowed from Repl. Holds no completion UI.
class ShellCompleter : public line::Completer
{
public:
    /// @brief Wire the completer to the shell's state and builtin
    /// registry.
    /// @param state Borrowed shell state (PATH, cwd); must outlive
    /// this.
    /// @param dispatcher Borrowed builtin registry (names,
    /// descriptions); must outlive this.
    ShellCompleter(
        std::unique_ptr<ShellState>& state,
        std::unique_ptr<builtins::BuiltinDispatcher>& dispatcher);
    ~ShellCompleter() override = default;
    ShellCompleter(const ShellCompleter&) = delete;
    ShellCompleter& operator=(const ShellCompleter&) = delete;
    ShellCompleter(ShellCompleter&&) = delete;
    ShellCompleter& operator=(ShellCompleter&&) = delete;

    /// @brief Complete the token ending at the cursor.
    /// @param line The full input buffer.
    /// @param cursor Byte index of the cursor within line.
    /// @return Command or path candidates and their replace index;
    /// empty if none.
    [[nodiscard]]
    line::Result Complete(const std::string& line,
                          std::size_t cursor) const override;

private:
    /// @brief Candidates for a command word: builtins + PATH executables.
    /// @param word The prefix typed so far.
    /// @param start Index where the word begins (the replace point).
    [[nodiscard]] line::Result CompleteCommand(const std::string& word,
                                               std::size_t start) const;

    /// @brief Candidates for a filename argument in the given directory.
    /// @param word The word so far, e.g. "/home/us" or "us".
    /// @param start Index where the word begins.
    [[nodiscard]] line::Result CompleteFile(const std::string& word,
                                            std::size_t start) const;

    /// @brief Borrowed shell state: source of PATH and the current
    /// directory.
    std::unique_ptr<ShellState>& m_state_;
    /// @brief Borrowed builtin registry: names and descriptions for
    /// commands.
    std::unique_ptr<builtins::BuiltinDispatcher>& m_dispatcher_;
};

} // namespace shell

#endif // SHELL_SHELL_COMPLETER_HPP