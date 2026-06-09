#ifndef EXEC_EXEC_EXCEPTION_HPP
#define EXEC_EXEC_EXCEPTION_HPP

#include <exception>
#include <string>

namespace exec
{

class ExecException : public std::exception
{
public:
    ExecException(const std::string& message, int errorCode);
    ~ExecException() = default;

    ExecException(const ExecException&) = default;
    ExecException& operator=(const ExecException&) = default;
    ExecException(ExecException&&) = default;
    ExecException& operator=(ExecException&&) = default;

    [[nodiscard]]
    const char* what() const noexcept override;

    [[nodiscard]]
    int GetErrorCode() const noexcept;

private:
    std::string m_message_;
    int m_errorCode_;

}; // ExecException

} // namespace exec

#endif // EXEC_EXEC_EXCEPTION_HPP