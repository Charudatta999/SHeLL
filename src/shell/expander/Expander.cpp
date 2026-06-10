#include "shell/expander/Expander.hpp"

#include "arithmetic/ArithmeticEngine.hpp"
#include "arithmetic/ArithmeticException.hpp"
#include "shell/ShellArithmeticVars.hpp"
#include "shell/ShellState.hpp"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace shell::expander
{
namespace
{
// Starting just past a "$((", scan the raw arithmetic expression up
// to the matching "))", tracking paren depth so nested parens are
// kept. Advances `pos` past the closing "))". Returns the inner
// expression text. Throws if the "((" is never closed.
std::string ReadArithBody(const std::string& word, std::size_t& pos)
{
    std::string expr;
    int depth = 0;
    while (pos < word.size())
    {
        char chr = word[pos];
        if (chr == '(')
        {
            ++depth;
            expr += chr;
            ++pos;
        }
        else if (chr == ')' && depth > 0)
        {
            --depth;
            expr += chr;
            ++pos;
        }
        else if (chr == ')') // depth == 0 -> start of closing "))"
        {
            if (pos + 1 < word.size() && word[pos + 1] == ')')
            {
                pos += 2; // consume the closing "))"
                return expr;
            }
            throw arithmetic::ArithmeticException(
                "malformed arithmetic expansion: expected ))");
        }
        else
        {
            expr += chr;
            ++pos;
        }
    }
    throw arithmetic::ArithmeticException(
        "unterminated arithmetic expansion $((");
}

} // namespace

std::vector<std::string> Expand(const std::string& word,
                                std::unique_ptr<ShellState>& state)
{
    std::string out;
    ShellArithmeticVars adapter(state);

    std::size_t pos = 0;
    while (pos < word.size())
    {
        // detect "$(("
        if (word[pos] == '$' && pos + 2 < word.size() &&
            word[pos + 1] == '(' && word[pos + 2] == '(')
        {
            pos += 3; // skip "$(("
            std::string expr = ReadArithBody(word, pos);
            std::int64_t result =
                arithmetic::engine::Evaluate(expr, adapter);
            out += std::to_string(result);
        }
        else if ((word[pos] == '$') &&
             (std::isalpha(word[pos+1]) || word[pos+1] == '_'))
        {
            std::size_t start = pos + 1;
            std::size_t end = start;
             while (end < word.size() && (std::isalnum(word[end]) || word[end] == '_'))
            {++end;}
            auto key = word.substr(start,end - start);
            auto value = state->GetVar(key);
            out+=value.value_or("");
            pos = end;
        }
        else
        {
            out += word[pos];
            ++pos;
        }
    }
    return {out};
}
} // namespace shell::expander
