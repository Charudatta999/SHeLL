#ifndef IO_EXEC_EXCEPTION_HPP
#define IO_EXEC_EXCEPTION_HPP

#include <exception>
#include <string>

namespace shell
{

class ShellException : public std::exception
{
private:
    std::string m_message_;
    int m_errorCode_;

public:
    ShellException(const std::string& message, int errorCode);

    [[nodiscard]]
    const char* what() const noexcept override;

    [[nodiscard]]
    int GetErrorCode() const noexcept;
}; // ExecException

} // namespace shell

#endif // IO_EXEC_EXCEPTION_HPP