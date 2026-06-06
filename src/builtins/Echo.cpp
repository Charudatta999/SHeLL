#include "builtins/BuiltInFunction.hpp"

#include <cstddef>
#include <string>
#include <unistd.h>

namespace
{

struct EchoOpts
{
    bool newline = true;
    bool escapes = false;
};

void ApplyFlagChars(const std::string& flag, EchoOpts& opts)
{
    for (char chr : flag)
    {
        switch (chr)
        {
            case 'n':
                opts.newline = false;
                break;
            case 'e':
                opts.escapes = true;
                break;
            case 'E':
                opts.escapes = false;
                break;
            default:
                break;
        }
    }
}

bool IsFlagToken(const std::string& arg)
{
    if (arg.size() < 2 || arg[0] != '-')
    {
        return false;
    }
    for (std::size_t i = 1; i < arg.size(); ++i)
    {
        if (arg[i] != 'n' && arg[i] != 'e' && arg[i] != 'E')
        {
            return false;
        }
    }
    return true;
}

size_t ParseFlags(const std::vector<std::string>& argv,
                  EchoOpts& opts)
{
    size_t index = 1;
    while (index < argv.size() && IsFlagToken(argv[index]))
    {
        ApplyFlagChars(argv[index], opts);
        ++index;
    }
    return index;
}

std::size_t
ReadOctal(const std::string& str, std::size_t pos, std::string& out)
{
    size_t digitsRead{0};
    size_t nextIndex{(pos + 2)};
    int value{0};

    while (digitsRead < 3 && nextIndex < str.size() &&
           (str[nextIndex] >= '0' && str[nextIndex] <= '7'))
    {
        value = (value * 8) + (str[nextIndex] - '0');
        ++digitsRead;
        ++nextIndex;
    }
    out += static_cast<char>(value);
    return nextIndex - 1;
}

int HexValue(char chr)
{
    if (chr >= '0' && chr <= '9')
        return chr - '0';
    if (chr >= 'a' && chr <= 'f')
        return chr - 'a' + 10;
    if (chr >= 'A' && chr <= 'F')
        return chr - 'A' + 10;
    return -1; // not a hex digit
}

std::size_t
ReadHex(const std::string& str, std::size_t pos, std::string& out)
{
    size_t digitsRead{0};
    size_t nextIndex{pos + 2};
    int value{0};

    while (digitsRead < 2 && nextIndex < str.size() &&
           HexValue(str[nextIndex]) != -1)
    {
        value = (value * 16) + HexValue(str[nextIndex]);
        ++digitsRead;
        ++nextIndex;
    }

    if (digitsRead == 0)
    {
        out += '\\';
        out += 'x';
        return pos + 1;
    }

    out += static_cast<char>(value);
    return nextIndex - 1;
}

size_t MatchEscapeSequence(const std::string& str,
                           size_t pos,
                           std::string& outString,
                           bool& stop)
{
    if (pos + 1 >= str.size())
    {
        outString += '\\';
        return pos;
    }
    char next = str[pos + 1];
    switch (next)
    {
        // simple escapes: append one byte, consumed through pos+1
        case 'n':
        {
            outString += '\n';
            return pos + 1;
        }
        case 't':
        {
            outString += '\t';
            return pos + 1;
        }
        case 'r':
        {
            outString += '\r';
            return pos + 1;
        }
        case '\\':
        {
            outString += '\\';
            return pos + 1;
        }
        case 'a':
        {
            outString += '\a';
            return pos + 1;
        }
        case 'b':
        {
            outString += '\b';
            return pos + 1;
        }
        case 'f':
        {
            outString += '\f';
            return pos + 1;
        }
        case 'v':
        {
            outString += '\v';
            return pos + 1;
        }
        case 'e':
        {
            outString += '\x1b';
            return pos + 1;
        }
        case 'c':
        {
            stop = true;
            return pos + 1;
        }

        // number escapes: fill in next stage
        case '0':
        {
            return ReadOctal(str, pos, outString);
        }
        case 'x':
        {
            return ReadHex(str, pos, outString);
        }
        default:
        {
            outString += '\\';
            outString += next;
            return pos + 1;
        }
    }
}

} // namespace

namespace builtins
{

int Echo(const std::vector<std::string>& argv,
         std::unique_ptr<BuiltinContext>& ctx)
{
    EchoOpts opts;
    auto startIndex = ParseFlags(argv, opts);

    std::string outString{};
    bool stop = false;
    for (size_t index = startIndex; index < argv.size(); index++)
    {
        if (index > startIndex)
        {
            outString += ' ';
        }
        const std::string& operand = argv[index];
        std::size_t innerIndex{0};
        while (innerIndex < operand.size())
        {
            char chr = operand[innerIndex];
            if (opts.escapes && chr == '\\')
            {
                innerIndex = MatchEscapeSequence(operand,
                                                 innerIndex,
                                                 outString,
                                                 stop) +
                             1;
                if (stop)
                {
                    break;
                }
            }
            else
            {
                outString += chr;
                ++innerIndex;
            }
        }
        if (stop)
        {
            break;
        }
    }
    write(ctx->outFd, outString.data(), outString.size());
    if (opts.newline && !stop)
    {
        write(ctx->outFd, "\n", 1);
    }
    return 0;
}
} // namespace builtins
