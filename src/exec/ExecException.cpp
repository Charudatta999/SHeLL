#include "exec/ExecException.hpp"

#include <cstring>
#include <string>

namespace exec
{

ExecException::ExecException(const std::string& message, int errorCode)
    : m_message_(message + ": " + std::strerror(errorCode))
    , m_errorCode_(errorCode)
{
}

const char* ExecException::what() const noexcept
{
    return m_message_.c_str();
}

int ExecException::GetErrorCode() const noexcept
{
    return m_errorCode_;
}

} // namespace io