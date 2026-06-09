#include "arithmetic/ArithmeticException.hpp"

namespace arithmetic
{
ArithmeticException::ArithmeticException(const std::string& message) : m_message_(message) {}

const char* ArithmeticException::what() const noexcept
{
    return m_message_.c_str();
}
} // namespace arithmetic