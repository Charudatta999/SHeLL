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

} // namespace parser
#endif // PARSER_PARSEREXECPTION_HPP