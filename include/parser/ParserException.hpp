#ifndef PARSER_PARSEREXECPTION_HPP
#define PARSER_PARSEREXECPTION_HPP

#include <exception>
#include <string>

namespace parser
{

class ParserException : public std::exception
{
public:
    ParserException(const std::string& message,
                    size_t line,
                    size_t col);
    ParserException(const std::string& message);
    ~ParserException() = default;
    ParserException(const ParserException&) = default;
    ParserException& operator=(const ParserException&) = default;
    ParserException(ParserException&&) = default;
    ParserException& operator=(ParserException&&) = default;

    [[nodiscard]]
    const char* what() const noexcept override;

private:
    std::string m_message_;
};

// Input is syntactically valid so far but ends too early (Eof where
// more is expected: open if/while, unterminated quote or $( ).
// The REPL catches this to keep reading lines (PS2) instead of
// reporting an error; any other ParserException is a real syntax
// error no further input can fix.
class IncompleteInputException : public ParserException
{
public:
    using ParserException::ParserException;
};

} // namespace parser
#endif // PARSER_PARSEREXECPTION_HPP