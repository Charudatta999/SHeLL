#ifndef ARITHMETIC_ARITHMETICVARS_HPP
#define ARITHMETIC_ARITHMETICVARS_HPP

#include <optional>
#include <string>

namespace arithmetic
{

class ArithmeticVars
{
public:
    ArithmeticVars () = default;
    virtual ~ArithmeticVars () = default;
    ArithmeticVars (const ArithmeticVars &) = delete;
    ArithmeticVars & operator=(const ArithmeticVars &) = delete;
    ArithmeticVars (ArithmeticVars &&) = delete;
    ArithmeticVars & operator=(ArithmeticVars &&) = delete;

    [[nodiscard]] virtual std::optional<std::string> Get(const std::string&) const = 0;
    virtual void Set(const std::string&, const std::string&) = 0;

};

} // namespace arithmetic
#endif // ARITHMETIC_ARITHMETICVARS_HPP