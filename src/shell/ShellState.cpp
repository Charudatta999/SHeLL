#include "shell/ShellState.hpp"

#include "parser/ast/commands/Function.hpp"
#include "shell/JobTable.hpp"
#include "shell/ShellException.hpp"
#include "utils/ErrorCodes.hpp"

#include <filesystem>
#include <memory>
#include <new>
#include <unistd.h>

namespace shell
{

ShellState::ShellState(
    const std::map<std::string, std::string>& globalVars)
    : m_cwd_{std::filesystem::current_path().string()}
    , m_vars_{globalVars}
    , m_exportedVars_{}
    , m_lastExitCode_{0}
    , m_runningFlag_{true}
    , m_shellOptions_{}
    , m_shellPid_{getpid()}
    , m_shellExitCode_{0}
    , m_functions_{}
    , m_jobsTable_(std::make_unique<JobTable>())
{
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

pid_t ShellState::GetPid() const
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

void ShellState::SetVar(const std::string& name,
                        const std::string& value) noexcept
{
    try
    {
        m_vars_[name] = value;
    }
    catch (const std::bad_alloc&)
    {
        // todo log when logger is intergrated
    }
}

void ShellState::UnSetVar(const std::string& varName) noexcept
{

    m_exportedVars_.erase(varName);
    m_vars_.erase(varName);
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

// ── Exit codes
// ───────────────────────────────────────────────────────────────

int ShellState::GetLastCommandExitCode() const
{
    return m_lastExitCode_;
}

int ShellState::GetShellExitCode() const
{
    return m_shellExitCode_;
}

void ShellState::SetLastExitCode(int code) noexcept
{
    m_lastExitCode_ = code;
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

} // namespace shell
