#ifndef SHELL_SHELL_STATE_HPP
#define SHELL_SHELL_STATE_HPP

#include <map>
#include <optional>
#include <set>
#include <string>
#include <sys/types.h>
#include <memory>

namespace parser::ast
{
    class AstNode;
}

namespace shell
{
class ShellState
{

public:
    ShellState(const std::map<std::string, std::string>& globalVars);
    ~ShellState();
    ShellState(const ShellState&) = delete;
    ShellState& operator=(const ShellState& shellState) = delete;
    ShellState(ShellState&& shellState) = delete;
    ShellState& operator=(ShellState&& shellState) = delete;

    [[nodiscard]]
    const std::string& GetCWD() const;

    [[nodiscard]]
    pid_t GetPid() const;

    void SetCWD(const std::string& currentDir);

    // Var env related functions
    [[nodiscard]]
    std::optional<std::string> GetVar(const std::string& varName) const;

    [[nodiscard]]
    std::map<std::string, std::string> GetLocalVars() const;

    void SetVar(const std::string& name, const std::string& value = "") noexcept;

    void UnSetVar(const std::string& varName) noexcept;

    [[nodiscard]]
    bool IsExported(const std::string& varName) const;

    void ExportVar(const std::string& varName) noexcept;

    [[nodiscard]]
    std::map<std::string, std::string> GetEnv() const;

    // exit code releated
    [[nodiscard]]
    int GetLastCommandExitCode() const;

    [[nodiscard]]
    int GetShellExitCode() const;

    void SetLastExitCode(int code) noexcept;

    // function related funcs
    void AddFunction(const std::string& name, std::unique_ptr<parser::ast::AstNode> body);

    [[nodiscard]]
   parser::ast::AstNode* GetFunctionBody(const std::string& functionName);

    [[nodiscard]]
    bool IsFunctionPresent(const std::string& functionName) const;

    void UnsetFunction(const std::string& functionName);

    // shell option related functions
    void SetOption(const std::string& option);

    void DisableOption(const std::string& option);

    [[nodiscard]]
    bool IsOptionEnabled(const std::string& option) const;

    // Running state — REPL loop control
    [[nodiscard]]
    bool IsRunning() const;
    void RequestExit(int exitCode) noexcept;

private:
    std::string m_cwd_;
    std::map<std::string, std::string> m_vars_;
    std::set<std::string> m_exportedVars_;
    int m_lastExitCode_;
    bool m_runningFlag_;
    std::map<std::string, bool> m_shellOptions_;
    pid_t m_shellPid_;
    int m_shellExitCode_;
    std::map<std::string, std::unique_ptr<parser::ast::AstNode>> m_functions_;
};
} // namespace shell
#endif // SHELL_SHELL_STATE_HPP