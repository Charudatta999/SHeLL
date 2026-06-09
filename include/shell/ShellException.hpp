#ifndef IO_EXEC_EXCEPTION_HPP
#define IO_EXEC_EXCEPTION_HPP

#include <exception>
#include <string>

namespace shell
{

class ShellException : public std::exception
{

public:
    ShellException(const std::string& message, int errorCode);
    ~ShellException() = default;

    ShellException(const ShellException&) = default;
    ShellException& operator=(const ShellException&) = default;
    ShellException(ShellException&&) = default;
    ShellException& operator=(ShellException&&) = default;

    [[nodiscard]]
    const char* what() const noexcept override;

    [[nodiscard]]
    int GetErrorCode() const noexcept;

private:
    std::string m_message_;
    int m_errorCode_;
}; // ExecException

} // namespace shell

#endif // IO_EXEC_EXCEPTION_HPP