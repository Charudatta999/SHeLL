#include "shell/expander/Expander.hpp"

#include "arithmetic/ArithmeticEngine.hpp"
#include "arithmetic/ArithmeticException.hpp"
#include "parser/ParserException.hpp"
#include "shell/ShellArithmeticVars.hpp"
#include "shell/ShellState.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <glob.h>
#include <memory>
#include <optional>
#include <pwd.h>
#include <unistd.h>
#include <string>
#include <system_error>
#include <utility>
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

std::optional<std::string> ResolveTilde(const std::string& name,
             std::unique_ptr<ShellState>& state)
{
    if (name.empty())
    {
        if (auto home = state->GetVar("HOME"))
            return home;
        if (const passwd* pw = ::getpwuid(::getuid()))
            return std::string(pw->pw_dir);
        return std::nullopt;
    }
    if (name == "+")
    {
        if (auto pwd = state->GetVar("PWD"))
            return pwd;
        return state->GetCWD();
    }
    if (name == "-")
        return state->GetVar("OLDPWD");
    if (const passwd* pw = ::getpwnam(name.c_str()))
        return std::string(pw->pw_dir);
    return std::nullopt;
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

std::optional<int> ToInteger(const std::string& str)
{
    if (str.empty())
        return std::nullopt;

    int value = 0;
    const char* end = str.data() + str.size();
    auto [ptr, ec] = std::from_chars(str.data(), end, value);
    if (ec == std::errc() && ptr == end)
        return value;
    return std::nullopt;
}

bool HasLeadingZero(const std::string& str)
{
    std::size_t i =
        (!str.empty() && (str[0] == '+' || str[0] == '-')) ? 1 : 0;
    return str.size() - i > 1 && str[i] == '0';
}

std::vector<std::string> SplitOnRange(const std::string& amble)
{
    std::vector<std::string> out;
    std::size_t start = 0;
    std::size_t pos;
    while ((pos = amble.find("..", start)) != std::string::npos)
    {
        out.push_back(amble.substr(start, pos - start));
        start = pos + 2;
    }
    out.push_back(amble.substr(start));
    return out;
}

std::vector<std::string> GenerateNumeric(int low,
                                         int high,
                                         int step,
                                         int width)
{
    std::vector<std::string> out;
    if (low <= high)
        for (int v = low; v <= high; v += step)
            out.push_back(std::format("{:0{}d}", v, width));
    else
        for (int v = low; v >= high; v -= step)
            out.push_back(std::format("{:0{}d}", v, width));
    return out;
}

std::vector<std::string> GenerateChar(char low, char high, int step)
{
    std::vector<std::string> out;
    if (low <= high)
        for (int c = low; c <= high; c += step)
            out.push_back(std::string(1, static_cast<char>(c)));
    else
        for (int c = low; c >= high; c -= step)
            out.push_back(std::string(1, static_cast<char>(c)));
    return out;
}

std::vector<std::string> ExpandRange(const std::string& amble)
{
    auto tokens = SplitOnRange(amble);
    if (tokens.size() < 2 || tokens.size() > 3)
        return {};

    const std::string& low = tokens[0];
    const std::string& high = tokens[1];

    int step = 1;
    if (tokens.size() == 3)
    {
        auto parsed = ToInteger(tokens[2]);
        if (!parsed)
            return {};
        step = *parsed;
    }
    if (step == 0)
        return {};
    step = std::abs(step);

    auto lowInt = ToInteger(low);
    auto highInt = ToInteger(high);
    if (lowInt && highInt)
    {
        const bool pad = HasLeadingZero(low) || HasLeadingZero(high);
        const int width =
            pad ? static_cast<int>(std::max(low.size(), high.size()))
                : 0;
        return GenerateNumeric(*lowInt, *highInt, step, width);
    }
    if (low.size() == 1 && high.size() == 1 &&
        std::isalpha(static_cast<unsigned char>(low[0])) &&
        std::isalpha(static_cast<unsigned char>(high[0])))
        return GenerateChar(low[0], high[0], step);

    return {};
}

std::vector<std::string> SplitTopLevelCommas(const std::string& amble)
{
    std::vector<std::string> parts;
    std::string cur;
    int depth = 0;
    for (char c : amble)
    {
        if (c == '{')
        {
            ++depth;
            cur += c;
        }
        else if (c == '}')
        {
            --depth;
            cur += c;
        }
        else if (c == ',' && depth == 0)
        {
            parts.push_back(cur);
            cur.clear();
        }
        else
        {
            cur += c;
        }
    }
    parts.push_back(cur);
    return parts;
}

std::vector<std::string> ExpandAmble(const std::string& amble)
{
    auto parts = SplitTopLevelCommas(amble);
    if (parts.size() >= 2)
        return parts;
    return ExpandRange(amble);
}

int MatchClose(const std::string& word, std::size_t openIdx)
{
    int depth = 0;
    for (std::size_t i = openIdx; i < word.size(); ++i)
    {
        if (word[i] == '{')
            ++depth;
        else if (word[i] == '}')
        {
            --depth;
            if (depth == 0)
                return static_cast<int>(i);
        }
    }
    return -1;
}

std::pair<int, int> FindFirstGroup(const std::string& word)
{
    for (std::size_t i = 0; i < word.size(); ++i)
        if (word[i] == '{')
        {
            int close = MatchClose(word, i);
            if (close != -1)
                return {static_cast<int>(i), close};
        }
    return {-1, -1};
}
} // namespace

std::vector<std::string> Expand(const std::string& word,
                                std::unique_ptr<ShellState>& state,  const CommandRunner& cmdRunner, bool assignment)
{
    std::string out;
    ShellArithmeticVars adapter(state);

    std::size_t pos = 0;
    while (pos < word.size())
    {
        // Tilde: only at word start, or after a literal ':' when the
        // word is an assignment value. The resolved directory goes
        // straight into `out` so it is not re-scanned for $-expansions
        if (word[pos] == '~' &&
            (pos == 0 || (assignment && word[pos - 1] == ':')))
        {
            std::size_t end = pos + 1;
            while (end < word.size() && word[end] != '/' &&
                   !(assignment && word[end] == ':'))
                ++end;
            auto dir = ResolveTilde(word.substr(pos + 1, end - pos - 1),
                                    state);
            if (dir)
            {
                out += *dir;
                pos = end;
                continue;
            }
            // Unresolved (~nosuchuser, ~- with OLDPWD unset): fall
            // through so the '~' is copied literally below.
        }

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
            if (!key.empty() &&
                std::all_of(key.begin(),
                            key.end(),
                            [](unsigned char chr)
                            { return std::isdigit(chr); }))
            {
                // ${N}: positional parameter, 1-based.
                const auto& params = state->GetPositionalParams();
                std::size_t idx = std::stoul(key);
                if (idx >= 1 && idx <= params.size())
                    out += params[idx - 1];
            }
            else
            {
                auto value = state->GetVar(key);
                out += value.value_or("");
            }
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
        else if (word[pos] == '$' && pos + 1 < word.size() &&
                 std::isdigit(word[pos + 1]))
        {
            // $1..$9: single digit only, bash semantics ($10 is
            // ${1}0). $0 falls in range and expands empty (the
            // shell name is not tracked yet).
            const auto& params = state->GetPositionalParams();
            auto idx =
                static_cast<std::size_t>(word[pos + 1] - '0');
            if (idx >= 1 && idx <= params.size())
                out += params[idx - 1];
            pos += 2;
        }
        else if (word[pos] == '$' && pos + 1 < word.size() &&
                 word[pos + 1] == '#')
        {
            out += std::to_string(state->GetPositionalParams().size());
            pos += 2;
        }
        else if (word[pos] == '$' && pos + 1 < word.size() &&
                 (word[pos + 1] == '@' || word[pos + 1] == '*'))
        {
            const auto& params = state->GetPositionalParams();
            for (std::size_t i = 0; i < params.size(); ++i)
            {
                if (i)
                    out += ' ';
                out += params[i];
            }
            pos += 2;
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

std::vector<std::string> BraceExpand(const std::string& word)
{
    auto [openIdx, closeIdx] = FindFirstGroup(word);
    if (openIdx == -1)
        return {word};

    const auto open = static_cast<std::size_t>(openIdx);
    const auto close = static_cast<std::size_t>(closeIdx);
    const std::string prefix = word.substr(0, open);
    const std::string amble = word.substr(open + 1, close - open - 1);
    const std::string suffix = word.substr(close + 1);

    auto parts = ExpandAmble(amble);
    auto tails = BraceExpand(suffix);

    std::vector<std::string> result;
    if (parts.empty())
    {
        for (const auto& tail : tails)
            result.push_back(prefix + "{" + amble + "}" + tail);
        return result;
    }

    for (const auto& part : parts)
        for (const auto& mid : BraceExpand(part))
            for (const auto& tail : tails)
                result.push_back(prefix + mid + tail);
    return result;
}
} // namespace shell::expander
