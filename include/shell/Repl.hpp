#ifndef SHELL_REPL_HPP
#define SHELL_REPL_HPP

#include "exec/Executor.hpp"
#include "line/History.hpp"
#include "line/LineEditor.hpp"
#include "line/Terminal.hpp"

#include <functional>
#include <memory>
#include <string>

namespace builtins
{
class BuiltinDispatcher;
}

namespace shell
{

class ShellState;

/// @brief Read-Eval-Print Loop: the interactive shell driver.
/// @details Run() loops: build the prompt, read a line, evaluate it, repeat
/// until `exit` or EOF (Ctrl-D). Owns the session state and the builtin
/// dispatcher, and constructs an Executor per evaluated line.
class Repl
{
public:
    Repl();
    ~Repl();
    Repl(const Repl&) = delete;
    Repl& operator=(const Repl&) = delete;
    Repl(Repl&&) = delete;
    Repl& operator=(Repl&&) = delete;

    /// @brief Run the loop until `exit` or EOF (Ctrl-D).
    /// @return The shell exit status.
    int Run();

private:
    /// @brief Build the prompt string from user, host, and current directory.
    std::string BuildPrompt();

    /// @brief Read one line without the line editor (non-interactive input).
    /// @param line Filled with the line read from stdin.
    /// @return False at end of input.
    bool ReadLine(std::string& line);

    /// @brief Tokenize, parse, and execute one input line.
    /// @param line The full line (may hold a multi-line command).
    /// @return The exit status of the evaluated line.
    int EvalLine(const std::string& line);

    /// @brief Session state: variables, cwd, jobs, signals.
    std::unique_ptr<ShellState> m_state_;
    /// @brief Registry of builtin commands.
    std::unique_ptr<builtins::BuiltinDispatcher> m_dispatcher_;
    /// @brief Runs a nested command and returns its captured output.
    std::function<std::string(const std::string&)> m_cmdRunner_;
    /// @brief Executes one parsed command tree.
    std::unique_ptr<exec::Executor> m_executor_;

    /// @brief Raw-mode terminal I/O.
    line::Terminal m_terminal_;
    /// @brief Persistent command history.
    line::History m_history_;
    /// @brief True when stdin is a TTY.
    bool m_interactive_ = false;
    /// @brief Tab-completion backend (a ShellCompleter), borrowed by the editor.
    std::unique_ptr<line::Completer> m_completer_;
    /// @brief The interactive line editor.
    std::unique_ptr<line::LineEditor> m_editor_;
};

} // namespace shell
#endif // SHELL_REPL_HPP
