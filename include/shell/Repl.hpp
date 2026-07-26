#ifndef SHELL_REPL_HPP
#define SHELL_REPL_HPP

#include "exec/Executor.hpp"
#include "line/History.hpp"
#include "line/LineEditor.hpp"
#include "line/Terminal.hpp"

#include <functional>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>

namespace builtins
{
class BuiltinDispatcher;
}

namespace shell
{
class ShellState;

// Read-Eval-Print Loop: the interactive driver.
//   Run() -> while not exiting: prompt -> read line -> eval -> repeat.
// Owns the shell session state and the builtin dispatcher; constructs an
// Executor per evaluated line.
class Repl
{
public:
    Repl();
    ~Repl();
    Repl(const Repl&) = delete;
    Repl& operator=(const Repl&) = delete;
    Repl(Repl&&) = delete;
    Repl& operator=(Repl&&) = delete;

    // Runs the loop until `exit` or EOF (Ctrl-D). Returns the shell exit status.
    int Run();

private:
    std::string BuildPrompt();
    bool ReadLine(std::string& line);       // non-interactive fallback
    int  EvalLine(const std::string& line);

    // Closes/reaps any process substitutions (<(cmd), >(cmd)) started
    // while expanding the command that just finished running.
    void CleanupProcSubs();

    std::unique_ptr<ShellState> m_state_;
    std::unique_ptr<builtins::BuiltinDispatcher> m_dispatcher_;
    std::function<std::string(const std::string&)> m_cmdRunner_;
    shell::expander::ProcSubRunner m_procSubRunner_;
    std::unique_ptr<exec::Executor> m_executor_;
    std::vector<pid_t> m_pendingProcSubPids_;

    line::Terminal m_terminal_;
    line::History m_history_;
    std::unique_ptr<line::LineEditor> m_editor_;
    bool m_interactive_ = false;
};

} // namespace shell
#endif // SHELL_REPL_HPP
