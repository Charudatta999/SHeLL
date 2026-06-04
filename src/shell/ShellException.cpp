#include "shell/ShellException.hpp"

namespace shell
{

ShellException::ShellException(const std::string& message, int errorCode)
    : m_message_(message)
    , m_errorCode_(errorCode)
{
}

const char* ShellException::what() const noexcept
{
    return m_message_.c_str();
}

int ShellException::GetErrorCode() const noexcept
{
    return m_errorCode_;
}

} // namespace shell
