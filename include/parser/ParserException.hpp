#ifndef PARSER_PARSEREXECPTION_HPP
#define PARSER_PARSEREXECPTION_HPP

#include <exception>
#include <string>
namespace parser
{

class ParserException : public std::exception
{
public:
    ParserException (const std::string& message, size_t line, size_t col);
    ~ParserException () = default;
    ParserException (const ParserException &) = delete;
    ParserException & operator=(const ParserException &) = delete;
    ParserException (ParserException &&) = delete;
    ParserException & operator=(ParserException &&) = delete;

    [[nodiscard]]
    const char* what() const noexcept override;

private:
    std::string m_message_;



};

} // namespace parser::ast
#endif // PARSER_PARSEREXECPTION_HPP