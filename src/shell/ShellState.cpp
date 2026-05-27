#include "shell/ShellState.hpp"

#include <unistd.h>

namespace shell
{

ShellState::ShellState()
    : m_cwd_{}
    , m_localVars_{}
    , m_globalVars_{}
    , m_exportedVars_{}
    , m_lastExitCode_{0}
    , m_runningFlag_{true}
    , m_shellOptions_{}
    , m_shellPid_{getpid()}
    , m_shellExitCode_{0}
    , m_functions_{}
{}

ShellState::~ShellState() = default;

// ── CWD ──────────────────────────────────────────────────────────────────────

std::string ShellState::GetCWD() const
{
    return {};
}

void ShellState::SetCWD(const std::string& currentDir)
{
}

// ── PID ──────────────────────────────────────────────────────────────────────

pid_t ShellState::GetPid() const
{
    return 0;
}

// ── Variables ────────────────────────────────────────────────────────────────

std::optional<std::string> ShellState::GetVar(const std::string& varName) const
{
    return std::nullopt;
}

std::vector<std::tuple<std::string, std::string>> ShellState::GetLocalVars() const
{
    return {};
}

std::vector<std::tuple<std::string, std::string>> ShellState::GetGlobalVars() const
{
    return {};
}

void ShellState::SetVar(const std::string& name, std::string value, bool isGlobal) noexcept
{
}

void ShellState::UnSetVar(const std::string& varName, bool isGlobal) noexcept
{
}

bool ShellState::IsExported(const std::string& varName) const
{
    return false;
}

void ShellState::ExportVar(const std::string& varName) noexcept
{
}

std::map<std::string, std::string> ShellState::GetEnv() const
{
    return {};
}

// ── Exit codes ───────────────────────────────────────────────────────────────

int ShellState::GetLastCommandExitCode() const
{
    return 0;
}

int ShellState::GetShellExitCode() const
{
    return 0;
}

void ShellState::SetLastExitCode(int code) noexcept
{
}

// ── Functions ────────────────────────────────────────────────────────────────

void ShellState::AddFunction(parser::FunctionNode function)
{
}

const parser::AstNode* ShellState::GetFunctionBody(const std::string& functionName) const
{
    return nullptr;
}

bool ShellState::IsFunctionPresent(const std::string& functionName) const
{
    return false;
}

void ShellState::UnsetFunction(const std::string& functionName)
{
}

// ── Shell options ─────────────────────────────────────────────────────────────

void ShellState::SetOption(const std::string& option)
{
}

void ShellState::DisableOption(const std::string& option)
{
}

bool ShellState::IsOptionEnabled(const std::string& option) const
{
    return false;
}

// ── Running state ─────────────────────────────────────────────────────────────

bool ShellState::IsRunning() const
{
    return false;
}

void ShellState::RequestExit(int exitCode) noexcept
{
}

} // namespace shell
