#include "shell/expander/Expander.hpp"

#include "arithmetic/ArithmeticEngine.hpp"
#include "arithmetic/ArithmeticException.hpp"
#include "parser/ParserException.hpp"
#include "shell/ShellArithmeticVars.hpp"
#include "shell/ShellState.hpp"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <glob.h>
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

std::string ReadWord(const std::string& word, std::size_t& pos)
{
    std::string out;
    size_t index = pos;
    while (index < word.size())
    {
        if (std::isalnum(word[index]) || word[index] == '_')
        {
            out += word[index];
            index++;
        }
        else
        {
            break;
        }
    }
    pos = index;
    return out;
}

std::string ReadCommandBody(const std::string& word, std::size_t& pos)
{
    std::string out;
    int depth = 0;
    while (pos < word.size())
    {
        char chr = word[pos];
        if (chr == '(')
        {
            ++depth;
            out += chr;
            ++pos;
        }
        else if (chr == ')' && depth > 0)
        {
            --depth;
            out += chr;
            ++pos;
        }
        else if (chr == ')') // depth == 0 -> start of closing ")"
        {
            ++pos; // consume the closing ")"
            return out;
        }
        else
        {
            out += chr;
            ++pos;
        }
    }
    throw parser::ParserException(
        "unterminated command substitution");
}
} // namespace

std::vector<std::string> Expand(const std::string& word,
                                std::unique_ptr<ShellState>& state,  const CommandRunner& cmdRunner)
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
        else if( word[pos] == '$' && word[pos+1] == '(')
        {
            pos += 2; // skip "$(" so the body scan starts inside
            out+= cmdRunner(ReadCommandBody(word, pos));
        }
        else if (word[pos] == '$' && word[pos + 1] == '{')
        {
            pos += 2;
            auto key = ReadWord(word, pos);
            auto value = state->GetVar(key);
            out += value.value_or("");
            if (pos < word.size() && word[pos] == '}')
                ++pos;
            else
            {
                throw parser::ParserException("bad substitution",
                                              0,
                                              0);
            }
        }
        else if ((word[pos] == '$') && (std::isalpha(word[pos + 1]) ||
                                        word[pos + 1] == '_'))
        {
            std::size_t start = pos + 1;
            std::size_t end = start;
            while (end < word.size() &&
                   (std::isalnum(word[end]) || word[end] == '_'))
            {
                ++end;
            }
            auto key = word.substr(start, end - start);
            auto value = state->GetVar(key);
            out += value.value_or("");
            pos = end;
        }
        else if (word[pos] == '$' && pos + 1 < word.size() &&
                 word[pos + 1] == '?')
        {
            pos += 2;
            out += std::to_string(state->GetLastCommandExitCode());
        }
        else if (word[pos] == '$' && pos + 1 < word.size() &&
                 word[pos + 1] == '$')
        {
            pos += 2;
            out += std::to_string(state->GetShellPid());
        }
        else
        {
            out += word[pos];
            ++pos;
        }
    }
    if (out.find_first_of("*?[") != std::string::npos)
    {
        glob_t globResult;
        if (glob(out.c_str(), 0, nullptr, &globResult) == 0 &&
            globResult.gl_pathc > 0)
        {
            std::vector<std::string> matches;
            for (std::size_t i = 0; i < globResult.gl_pathc; ++i)
            {
                matches.push_back(globResult.gl_pathv[i]);
            }
            globfree(&globResult);
            return matches;
        }
        globfree(&globResult);
    }
    return {out};
}
} // namespace shell::expander
