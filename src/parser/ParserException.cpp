#include "parser/ParserException.hpp"

#include <cstring>
#include <string>

namespace parser
{

ParserException::ParserException(const std::string& message,
                                 size_t line,
                                 size_t col)
    : m_message_(message + ": " +
                 "at line : " + std::to_string(line) +
                 " at col: " + std::to_string(col))
{
}

const char* ParserException::what() const noexcept
{
    return m_message_.c_str();
}

} // namespace parser