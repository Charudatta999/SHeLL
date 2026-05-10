#include "io/IOException.hpp"

#include <cstring>
#include <string>

namespace io
{

IOException::IOException(const std::string& message, int errorCode)
    : m_message_(message + ": " + std::strerror(errorCode))
    , m_errorCode_(errorCode)
{
}

const char* IOException::what() const noexcept
{
    return m_message_.c_str();
}

int IOException::GetErrorCode() const noexcept
{
    return m_errorCode_;
}

} // namespace io