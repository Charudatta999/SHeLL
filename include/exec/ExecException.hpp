#ifndef EXEC_EXEC_EXCEPTION_HPP
#define EXEC_EXEC_EXCEPTION_HPP

#include <exception>
#include <string>

namespace exec
{

class ExecException : public std::exception
{
private:
    std::string m_message_;
    int m_errorCode_;

public:
    ExecException(const std::string& message, int errorCode);

    [[nodiscard]]
    const char* what() const noexcept override;

    [[nodiscard]]
    int GetErrorCode() const noexcept;
}; // ExecException

} // namespace exec

#endif // EXEC_EXEC_EXCEPTION_HPP