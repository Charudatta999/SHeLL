#include "line/Terminal.hpp"

#include <format>
#include <string>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>

namespace line
{

Terminal::Terminal() : m_origTermios_(), m_rawMode_(false) {}

Terminal::~Terminal()
{
    if (m_rawMode_)
        DisableRawMode();
}

void Terminal::EnableRawMode()
{
    if (m_rawMode_)
        return;

    tcgetattr(STDIN_FILENO, &m_origTermios_);

    struct termios raw = m_origTermios_;
    raw.c_iflag &= static_cast<tcflag_t>(~(BRKINT | ICRNL | INPCK | ISTRIP | IXON));
    raw.c_cflag |= static_cast<tcflag_t>(CS8);
    raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON | ISIG | IEXTEN));
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    m_rawMode_ = true;
}

void Terminal::DisableRawMode()
{
    if (!m_rawMode_)
        return;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_origTermios_);
    m_rawMode_ = false;
}

KeyEvent Terminal::ReadKey()
{
    char byte = '\0';
    while (read(STDIN_FILENO, &byte, 1) != 1)
    {
    }

    if (byte == '\x1b')
        return ParseEscapeSequence();

    if (byte == '\r' || byte == '\n')
        return {.key = Key::Enter};
    if (byte == '\t')
        return {.key = Key::Tab};
    if (byte == '\x7f')
        return {.key = Key::Backspace};

    if (byte >= 1 && byte <= 26)
    {
        switch (byte)
        {
            case 1:
                return {.key = Key::CtrlA};
            case 2:
                return {.key = Key::CtrlB};
            case 3:
                return {.key = Key::CtrlC};
            case 4:
                return {.key = Key::CtrlD};
            case 5:
                return {.key = Key::CtrlE};
            case 6:
                return {.key = Key::CtrlF};
            case 11:
                return {.key = Key::CtrlK};
            case 12:
                return {.key = Key::CtrlL};
            case 14:
                return {.key = Key::CtrlN};
            case 16:
                return {.key = Key::CtrlP};
            case 18:
                return {.key = Key::CtrlR};
            case 21:
                return {.key = Key::CtrlU};
            case 23:
                return {.key = Key::CtrlW};
            case 25:
                return {.key = Key::CtrlY};
            case 26:
                return {.key = Key::Tab};
            default:
                return {.key = Key::Unknown};
        }
    }

    return {.key = Key::Char, .ch = byte};
}

bool Terminal::HasPendingInput(int timeoutMs)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    struct timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000L;

    return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &timeout) > 0;
}

// A terminal escape parser is inherently one large branch on the byte
// sequence; splitting it would obscure the protocol, so the complexity is
// suppressed rather than refactored.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
KeyEvent Terminal::ParseEscapeSequence()
{
    if (!HasPendingInput(50))
        return {.key = Key::Escape};

    char seq0 = '\0';
    if (read(STDIN_FILENO, &seq0, 1) != 1)
        return {.key = Key::Escape};

    if (seq0 == '[')
    {
        if (!HasPendingInput(50))
            return {.key = Key::Escape};

        char seq1 = '\0';
        if (read(STDIN_FILENO, &seq1, 1) != 1)
            return {.key = Key::Escape};

        if (seq1 >= '0' && seq1 <= '9')
        {
            if (!HasPendingInput(50))
                return {.key = Key::Escape};

            char seq2 = '\0';
            if (read(STDIN_FILENO, &seq2, 1) != 1)
                return {.key = Key::Escape};

            if (seq2 == '~')
            {
                switch (seq1)
                {
                    case '1': return {.key = Key::Home};
                    case '3': return {.key = Key::Delete};
                    case '4': return {.key = Key::End};
                    default:  return {.key = Key::Unknown};
                }
            }

            if (seq2 == ';')
            {
                char mod = '\0';
                char dir = '\0';
                if (!HasPendingInput(50))
                    return {.key = Key::Escape};
                if (read(STDIN_FILENO, &mod, 1) != 1)
                    return {.key = Key::Escape};
                if (!HasPendingInput(50))
                    return {.key = Key::Escape};
                if (read(STDIN_FILENO, &dir, 1) != 1)
                    return {.key = Key::Escape};

                if (mod == '5')
                {
                    if (dir == 'C') return {.key = Key::CtrlRight};
                    if (dir == 'D') return {.key = Key::CtrlLeft};
                }
                return {.key = Key::Unknown};
            }
            return {.key = Key::Unknown};
        }

        switch (seq1)
        {
            case 'A': return {.key = Key::Up};
            case 'B': return {.key = Key::Down};
            case 'C': return {.key = Key::Right};
            case 'D': return {.key = Key::Left};
            case 'H': return {.key = Key::Home};
            case 'F': return {.key = Key::End};
            case 'Z': return {.key = Key::ShiftTab}; // CSI Z = BackTab
            default:  return {.key = Key::Unknown};
        }
    }

    if (seq0 == 'O')
    {
        if (!HasPendingInput(50))
            return {.key = Key::Escape};

        char seq1 = '\0';
        if (read(STDIN_FILENO, &seq1, 1) != 1)
            return {.key = Key::Escape};

        switch (seq1)
        {
            case 'H': return {.key = Key::Home};
            case 'F': return {.key = Key::End};
            default:  return {.key = Key::Unknown};
        }
    }

    return {.key = Key::Escape};
}

bool Terminal::IsRawMode() const
{
    return m_rawMode_;
}

void Terminal::MoveCursorLeft(int count)
{
    if (count <= 0)
        return;
    const std::string seq = std::format("\x1b[{}D", count);
    write(STDOUT_FILENO, seq.data(), seq.size());
}

void Terminal::MoveCursorRight(int count)
{
    if (count <= 0)
        return;
    const std::string seq = std::format("\x1b[{}C", count);
    write(STDOUT_FILENO, seq.data(), seq.size());
}

void Terminal::MoveCursorToCol(int col)
{
    const std::string seq = std::format("\x1b[{}G", col + 1);
    write(STDOUT_FILENO, seq.data(), seq.size());
}

void Terminal::ClearToEndOfLine()
{
    write(STDOUT_FILENO, "\x1b[K", 3);
}

void Terminal::MoveCursorUp(int count)
{
    if (count <= 0)
        return;
    const std::string seq = std::format("\x1b[{}A", count);
    write(STDOUT_FILENO, seq.data(), seq.size());
}

void Terminal::MoveCursorDown(int count)
{
    if (count <= 0)
        return;
    const std::string seq = std::format("\x1b[{}B", count);
    write(STDOUT_FILENO, seq.data(), seq.size());
}

void Terminal::ClearBelow()
{
    // Erase from the cursor to the end of the screen (wipes the menu).
    write(STDOUT_FILENO, "\x1b[0J", 4);
}

void Terminal::ClearScreen()
{
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
}

int Terminal::GetWidth()
{
    struct winsize winsz{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsz) == 0 && winsz.ws_col > 0)
        return winsz.ws_col;
    return 80;
}

int Terminal::GetHeight()
{
    struct winsize winsz{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsz) == 0 && winsz.ws_row > 0)
        return winsz.ws_row;
    return 24;
}

} // namespace line
