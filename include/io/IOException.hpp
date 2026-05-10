#ifndef IO_IOEXCEPTION_HPP
#define IO_IOEXCEPTION_HPP

#include <exception>
#include <string>

namespace io
{

class IOException : public std::exception
{
private:
    std::string m_message_;
    int m_errorCode_;

public:
    IOException(const std::string& message, int errorCode);

    [[nodiscard]]
    const char* what() const noexcept override;

    [[nodiscard]]
    int GetErrorCode() const noexcept;
}; // IOException

} // namespace io

#endif // IO_IOEXCEPTION_HPP