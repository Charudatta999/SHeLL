#ifndef LINE_TERMINAL_HPP
#define LINE_TERMINAL_HPP

#include "line/KeyEvent.hpp"

#include <termios.h>

namespace line
{

class Terminal
{
public:
    Terminal();
    ~Terminal();
    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;
    Terminal(Terminal&&) = delete;
    Terminal& operator=(Terminal&&) = delete;

    void EnableRawMode();
    void DisableRawMode();
    [[nodiscard]]
    bool IsRawMode() const;

    KeyEvent ReadKey();

    void MoveCursorLeft(int count = 1);
    void MoveCursorRight(int count = 1);
    void MoveCursorToCol(int col);
    void ClearToEndOfLine();
    void ClearScreen();

    [[nodiscard]]
    int GetWidth();

private:
    KeyEvent ParseEscapeSequence();
    bool HasPendingInput(int timeoutMs);

    struct termios m_origTermios_;
    bool m_rawMode_;
};

} // namespace line
#endif // LINE_TERMINAL_HPP
