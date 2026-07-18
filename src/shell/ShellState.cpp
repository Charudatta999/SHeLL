#include "shell/ShellState.hpp"

#include "parser/ast/commands/Function.hpp"
#include "shell/JobTable.hpp"
#include "shell/ShellException.hpp"
#include "signals/SignalManager.hpp"
#include "utils/ErrorCodes.hpp"

#include <filesystem>
#include <memory>
#include <new>
#include <termios.h>
#include <unistd.h>

namespace shell
{

ShellState::ShellState(
    const std::map<std::string, std::string>& globalVars)
    : m_cwd_{std::filesystem::current_path().string()}
    , m_vars_{globalVars}
    , m_exportedVars_{}
    , m_lastCommandExitCode_{0}
    , m_runningFlag_{true}
    , m_shellOptions_{}
    , m_shellPid_{getpid()}
    , m_shellExitCode_{0}
    , m_functions_{}
    , m_jobsTable_(std::make_unique<JobTable>())
    // Off until an interactive, tty-owning shell turns it on in Repl::Run.
    // Tests and non-interactive use must not attempt terminal handoff.
    , m_jobControl_(false)
    , m_sigMgr_(std::make_unique<signals::SignalManager>())
    ,m_savedTermios_{ }
    ,m_hasSavedTermios_(false)
{
    // Variables inherited from the process environment are exported,
    // matching bash: they came from environ and flow back to children.
    for (const auto& itr : globalVars)
        m_exportedVars_.insert(itr.first);
}

ShellState::~ShellState() = default;

// ── CWD
// ──────────────────────────────────────────────────────────────────────

const std::string& ShellState::GetCWD() const
{
    return m_cwd_;
}

void ShellState::SetCWD(const std::string& currentDir)
{
    m_cwd_ = currentDir;
}

// ── PID
// ──────────────────────────────────────────────────────────────────────

pid_t ShellState::GetShellPid() const
{
    return m_shellPid_;
}

// ── Variables
// ────────────────────────────────────────────────────────────────

std::optional<std::string>
ShellState::GetVar(const std::string& varName) const
{
    auto itr = m_vars_.find(varName);
    if (itr != m_vars_.end())
    {
        return itr->second;
    }
    return std::nullopt;
}

std::map<std::string, std::string> ShellState::GetLocalVars() const
{
    std::map<std::string, std::string> localVars;
    for (const auto& itr : m_vars_)
    {
        if (!m_exportedVars_.count(itr.first))
        {
            localVars.emplace(itr);
        }
    }
    return localVars;
}

bool ShellState::SetVar(const std::string& name,
                        const std::string& value) noexcept
{
    if (m_readonlyVars_.count(name))
        return false;
    try
    {
        m_vars_[name] = value;
        return true;
    }
    catch (const std::bad_alloc&)
    {
        // todo log when logger is intergrated
        return false;
    }
}

bool ShellState::UnSetVar(const std::string& varName) noexcept
{
    if (m_readonlyVars_.count(varName))
        return false;
    m_exportedVars_.erase(varName);
    m_vars_.erase(varName);
    return true;
}

bool ShellState::IsExported(const std::string& varName) const
{
    return m_exportedVars_.count(varName);
}

void ShellState::ExportVar(const std::string& varName) noexcept
{
    try
    {
        m_exportedVars_.insert(varName);
    }
    catch (const std::bad_alloc&)
    {
        // todo log when logger is intergrated
    }
}

const std::set<std::string>& ShellState::GetExportedNames() const
{
    return m_exportedVars_;
}

std::map<std::string, std::string> ShellState::GetEnv() const
{
    std::map<std::string, std::string> envVars;
    for (const auto& itr : m_vars_)
    {
        if (m_exportedVars_.count(itr.first))
        {
            envVars.emplace(itr);
        }
    }
    return envVars;
}

// ── Readonly variables
// ───────────────────────────────────────────────────

void ShellState::MarkReadonly(const std::string& varName) noexcept
{
    try
    {
        m_readonlyVars_.insert(varName);
    }
    catch (const std::bad_alloc&)
    {
        // todo log when logger is intergrated
    }
}

bool ShellState::IsReadonly(const std::string& varName) const
{
    return m_readonlyVars_.count(varName);
}

const std::set<std::string>& ShellState::GetReadonlyVars() const
{
    return m_readonlyVars_;
}

// ── Positional parameters
// ────────────────────────────────────────────────

void ShellState::SetPositionalParams(std::vector<std::string> params)
{
    m_positionalParams_ = std::move(params);
}

const std::vector<std::string>&
ShellState::GetPositionalParams() const
{
    return m_positionalParams_;
}

// ── Exit codes
// ───────────────────────────────────────────────────────────────

int ShellState::GetLastCommandExitCode() const
{
    return m_lastCommandExitCode_;
}

int ShellState::GetShellExitCode() const
{
    return m_shellExitCode_;
}

void ShellState::SetLastCommandExitCode(int code) noexcept
{
    m_lastCommandExitCode_ = code;
}

// ── Functions
// ────────────────────────────────────────────────────────────────

void ShellState::AddFunction(
    const std::string& name,
    std::unique_ptr<parser::ast::AstNode> body)
{
    m_functions_.emplace(name, std::move(body));
}

parser::ast::AstNode*
ShellState::GetFunctionBody(const std::string& functionName)
{
    try
    {
        auto it = m_functions_.find(functionName);
        if (it == m_functions_.end())
        {
            return nullptr;
        }
        return it->second.get();
    }
    catch (const std::out_of_range&)
    {
        throw ShellException("function not found", VALUE_NOT_FOUND);
    }
}

bool ShellState::IsFunctionPresent(
    const std::string& functionName) const
{
    return m_functions_.count(functionName);
}

void ShellState::UnsetFunction(const std::string& functionName)
{
    m_functions_.erase(functionName);
}

// ── Shell options
// ─────────────────────────────────────────────────────────────

void ShellState::SetOption(const std::string& option)
{
    m_shellOptions_[option] = true;
}

void ShellState::DisableOption(const std::string& option)
{
    m_shellOptions_[option] = false;
}

bool ShellState::IsOptionEnabled(const std::string& option) const
{
    auto itr = m_shellOptions_.find(option);
    return itr != m_shellOptions_.end() && itr->second;
}

// ── Running state
// ─────────────────────────────────────────────────────────────

bool ShellState::IsRunning() const
{
    return m_runningFlag_;
}

void ShellState::RequestExit(int exitCode) noexcept
{
    m_runningFlag_ = false;
    m_shellExitCode_ = exitCode;
}

std::unique_ptr<JobTable>& ShellState::GetJobs()
{
    return m_jobsTable_;
}

void ShellState::EnableJobControl(bool enable)
{
    m_jobControl_ = enable;
}

bool ShellState::IsJobControlEnabled() const
{
    return m_jobControl_;
}

const std::unique_ptr<signals::SignalManager>& ShellState::GetSignalMgr()
{
    return m_sigMgr_;
}

bool ShellState::HasSavedTerminalModes() const
{
    return m_hasSavedTermios_;
}

bool ShellState::RestoreTerminalModes()
{
    if (!isatty(STDIN_FILENO) || !m_hasSavedTermios_)
        return false;

    return tcsetattr(STDIN_FILENO, TCSADRAIN, &m_savedTermios_) != -1;
}

void ShellState::SaveTerminalModes()
{
    if (!isatty(STDIN_FILENO))
        return;
    auto res = tcgetattr(STDIN_FILENO, &m_savedTermios_);
    m_hasSavedTermios_ = (res != -1);
}
} // namespace shell
