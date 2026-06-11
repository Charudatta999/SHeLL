#ifndef SHELL_REPL_HPP
#define SHELL_REPL_HPP

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
    void PrintPrompt();                     // write the prompt to stdout
    bool ReadLine(std::string& line);       // read one line; false on EOF
    int  EvalLine(const std::string& line); // tokenize -> parse -> execute

    std::unique_ptr<ShellState> m_state_;
    std::unique_ptr<builtins::BuiltinDispatcher> m_dispatcher_;
    std::function<std::string(const std::string&)> m_cmdRunner_;
};

} // namespace shell
#endif // SHELL_REPL_HPP
