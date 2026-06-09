#ifndef ARITHMETIC_ARITHMETICENGINE_HPP
#define ARITHMETIC_ARITHMETICENGINE_HPP
#include <cstdint>
#include <string>

namespace arithmetic
{
class ArithmeticVars;

namespace engine
{

std::int64_t Evaluate(const std::string& expression,
                      ArithmeticVars& vars);
}

} // namespace arithmetic
#endif // ARITHMETIC_ARITHMETICENGINE_HPP