#ifndef SHELL_SHELL_STATE_HPP
#define SHELL_SHELL_STATE_HPP

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <sys/types.h>
#include <termios.h>
#include <vector>

namespace parser::ast
{
class AstNode;
}

namespace signals
{
class SignalManager;
}

namespace shell
{
class JobTable;

/// @brief Central, per-session state for one running shell.
///
/// Owns everything the REPL and executor need to share: the current
/// working directory, the variable environment (with its exported
/// subset), the last and final exit codes, user-defined functions,
/// shell options, the job table, the signal manager, and the saved
/// terminal modes. A single instance lives for the lifetime of the
/// shell and is passed by reference to the pieces that need it.
///
/// Non-copyable and non-movable: it holds unique ownership of the job
/// table and signal manager, and other components hold references to it.
class ShellState
{

public:
    /// @brief Construct the session state.
    /// @param globalVars Initial variables (typically the inherited
    ///        process environment) used to seed the variable map.
    ShellState(const std::map<std::string, std::string>& globalVars);
    ~ShellState();
    ShellState(const ShellState&) = delete;
    ShellState& operator=(const ShellState& shellState) = delete;
    ShellState(ShellState&& shellState) = delete;
    ShellState& operator=(ShellState&& shellState) = delete;

    /// @brief Get the shell's current working directory.
    /// @return The absolute path of the current directory.
    [[nodiscard]]
    const std::string& GetCWD() const;

    /// @brief Get the process ID of the shell itself.
    /// @return The shell's PID.
    [[nodiscard]]
    pid_t GetShellPid() const;

    /// @brief Set the shell's current working directory.
    /// @param currentDir The new working directory path.
    void SetCWD(const std::string& currentDir);

    // Variable environment

    /// @brief Look up a variable's value.
    /// @param varName Name of the variable.
    /// @return The value, or std::nullopt if the variable is unset.
    [[nodiscard]]
    std::optional<std::string>
    GetVar(const std::string& varName) const;

    /// @brief Get a copy of all variables, exported and non-exported.
    /// @return A name to value map of every variable in the session.
    [[nodiscard]]
    std::map<std::string, std::string> GetLocalVars() const;

    /// @brief Set (or overwrite) a variable's value.
    /// @param name Name of the variable.
    /// @param value Value to assign; defaults to empty.
    /// @return false if the variable is readonly (value unchanged).
    bool SetVar(const std::string& name,
                const std::string& value = "") noexcept;

    /// @brief Remove a variable if it exists.
    /// @param varName Name of the variable to unset.
    /// @return false if the variable is readonly (not removed).
    bool UnSetVar(const std::string& varName) noexcept;

    /// @brief Test whether a variable is marked for export.
    /// @param varName Name of the variable.
    /// @return true if the variable is in the exported set.
    [[nodiscard]]
    bool IsExported(const std::string& varName) const;

    /// @brief Mark a variable for export to child processes.
    /// @param varName Name of the variable to export.
    void ExportVar(const std::string& varName) noexcept;

    /// @brief Clear the export mark without unsetting the value.
    /// @param varName Name of the variable to unexport.
    void UnexportVar(const std::string& varName) noexcept;

    /// @brief Get the set of names marked for export.
    /// @return The exported-name set (names may have no value yet).
    [[nodiscard]]
    const std::set<std::string>& GetExportedNames() const;

    /// @brief Build the environment passed to child processes.
    /// @return A name to value map of only the exported variables.
    [[nodiscard]]
    std::map<std::string, std::string> GetEnv() const;

    // Readonly variables

    /// @brief Mark a variable immutable; SetVar/UnSetVar then
    ///        refuse it.
    /// @param varName Name of the variable to protect.
    void MarkReadonly(const std::string& varName) noexcept;

    /// @brief Test whether a variable is readonly.
    /// @param varName Name of the variable.
    /// @return true if the variable is in the readonly set.
    [[nodiscard]]
    bool IsReadonly(const std::string& varName) const;

    /// @brief Get the set of readonly variable names.
    /// @return The readonly-name set.
    [[nodiscard]]
    const std::set<std::string>& GetReadonlyVars() const;

    // Positional parameters ($1.., set by `set -- args`) and $0

    /// @brief Replace all positional parameters atomically.
    /// @param params The new parameter list; $1 is params[0].
    void SetPositionalParams(std::vector<std::string> params);

    /// @brief Get the current positional parameters.
    /// @return The parameter list; $1 is index 0.
    [[nodiscard]]
    const std::vector<std::string>& GetPositionalParams() const;

    /// @brief Set the shell/script name expanded by `$0`.
    /// @param name Typically `argv[0]`; defaults to `"shell"`.
    void SetArg0(std::string name);

    /// @brief Get the name expanded by `$0` / `${0}`.
    [[nodiscard]]
    const std::string& GetArg0() const;

    // Exit codes

    /// @brief Get the exit status of the most recent command ($?).
    /// @return The last command's exit code.
    [[nodiscard]]
    int GetLastCommandExitCode() const;

    /// @brief Get the exit code the shell will return on termination.
    /// @return The shell's own exit code.
    [[nodiscard]]
    int GetShellExitCode() const;

    /// @brief Record the exit status of the most recent command.
    /// @param code The command's exit code.
    void SetLastCommandExitCode(int code) noexcept;

    // User-defined functions

    /// @brief Register a shell function by name.
    /// @param name Function name.
    /// @param body Parsed body of the function; ownership is taken.
    void AddFunction(const std::string& name,
                     std::unique_ptr<parser::ast::AstNode> body);

    /// @brief Get the parsed body of a registered function.
    /// @param functionName Name of the function.
    /// @return Pointer to the function body, or nullptr if not defined.
    ///         Ownership stays with the ShellState.
    [[nodiscard]]
    parser::ast::AstNode*
    GetFunctionBody(const std::string& functionName);

    /// @brief Test whether a function is defined.
    /// @param functionName Name of the function.
    /// @return true if a function with that name is registered.
    [[nodiscard]]
    bool IsFunctionPresent(const std::string& functionName) const;

    /// @brief Remove a registered function if it exists.
    /// @param functionName Name of the function to remove.
    void UnsetFunction(const std::string& functionName);

    // Shell options

    /// @brief Enable a shell option.
    /// @param option Name of the option to turn on.
    void SetOption(const std::string& option);

    /// @brief Disable a shell option.
    /// @param option Name of the option to turn off.
    void DisableOption(const std::string& option);

    /// @brief Test whether a shell option is enabled.
    /// @param option Name of the option.
    /// @return true if the option is currently on.
    [[nodiscard]]
    bool IsOptionEnabled(const std::string& option) const;

    // Running state and REPL loop control

    /// @brief Test whether the REPL should keep running.
    /// @return true until an exit has been requested.
    [[nodiscard]]
    bool IsRunning() const;

    /// @brief Ask the REPL to stop after the current iteration.
    /// @param exitCode The exit code the shell should return.
    void RequestExit(int exitCode) noexcept;

    // Job Control and signals

    /// @brief Access the job table.
    /// @return Reference to the owned JobTable.
    std::unique_ptr<JobTable>& GetJobs();

    /// @brief Turn job control (monitor mode) on or off.
    /// @param enable true to enable job control, false to disable it.
    void EnableJobControl(bool enable);

    /// @brief Test whether job control is currently enabled.
    /// @return true if job control is on.
    [[nodiscard]]
    bool IsJobControlEnabled() const;

    /// @brief Record that this shell was started as a login shell
    ///        (argv[0] beginning with '-', or --login/-l).
    /// @param login true if the shell is a login shell.
    void SetLoginShell(bool login);

    /// @brief Test whether this is a login shell. Guards operations
    ///        that would strand the user's session, e.g. `suspend`.
    /// @return true if the shell is a login shell.
    [[nodiscard]]
    bool IsLoginShell() const;

    /// @brief Access the signal manager.
    /// @return Reference to the owned SignalManager.
    const std::unique_ptr<signals::SignalManager>& GetSignalMgr();

    // Terminal modes

    /// @brief Snapshot the terminal's current modes as the known-good
    ///        baseline. Call once at interactive startup, while the
    ///        terminal is still sane, before entering raw mode. Records
    ///        the modes only if the read succeeds.
    void SaveTerminalModes();

    /// @brief Restore the terminal to the saved baseline modes. Call
    ///        after reclaiming the terminal from a foreground job so
    ///        a job that left it in a broken state cannot corrupt the
    ///        prompt. A no-op if no modes were ever saved.
    /// @return true if a restore is success
    bool RestoreTerminalModes();

    /// @brief Test whether a terminal baseline has been saved.
    /// @return true if SaveTerminalModes previously succeeded.
    [[nodiscard]]
    bool HasSavedTerminalModes() const;

    // Process substitution bookkeeping (<(cmd), >(cmd))

    /// @brief A live process substitution: the parent-side fd handed
    ///        out as /dev/fd/N, and the child pid running its body.
    struct ProcSub
    {
        int fd;
        pid_t pid;
    };

    /// @brief Register a process substitution started during word
    ///        expansion, so its fd can be closed and its child reaped
    ///        once the foreground command has been launched.
    void AddProcSub(int fd, pid_t pid);

    /// @brief Hand the caller every process substitution registered
    ///        since the last call, clearing the pending list.
    [[nodiscard]]
    std::vector<ProcSub> TakeProcSubs();

private:
    std::string m_cwd_;
    std::map<std::string, std::string> m_vars_;
    std::set<std::string> m_exportedVars_;
    std::set<std::string> m_readonlyVars_;
    std::vector<std::string> m_positionalParams_;
    std::string m_arg0_;
    int m_lastCommandExitCode_;
    bool m_runningFlag_;
    std::map<std::string, bool> m_shellOptions_;
    pid_t m_shellPid_;
    int m_shellExitCode_;
    std::map<std::string, std::unique_ptr<parser::ast::AstNode>>
        m_functions_;
    std::unique_ptr<JobTable> m_jobsTable_;
    bool m_jobControl_;
    bool m_loginShell_;
    std::unique_ptr<signals::SignalManager> m_sigMgr_;
    struct termios m_savedTermios_;
    bool m_hasSavedTermios_;
    std::vector<ProcSub> m_procSubs_;
};
} // namespace shell
#endif // SHELL_SHELL_STATE_HPP
