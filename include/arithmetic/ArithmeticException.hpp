#ifndef ARITHMETIC_ARITHMETICEXCEPTION_HPP
#define ARITHMETIC_ARITHMETICEXCEPTION_HPP
#include <exception>
#include <string>

namespace arithmetic
{

class ArithmeticException : public std::exception
{
public:
    ArithmeticException(const std::string& message);
    ~ArithmeticException() = default;
    ArithmeticException(const ArithmeticException&) = default;
    ArithmeticException&
    operator=(const ArithmeticException&) = default;
    ArithmeticException(ArithmeticException&&) = default;
    ArithmeticException& operator=(ArithmeticException&&) = default;

    [[nodiscard]]
    const char* what() const noexcept override;

private:
    std::string m_message_;
};

} // namespace arithmetic
#endif // ARITHMETIC_ARITHMETICEXCEPTION_HPP