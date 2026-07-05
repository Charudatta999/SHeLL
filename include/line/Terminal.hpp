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

    /// @brief Move the cursor left @p count columns.
    void MoveCursorLeft(int count = 1);
    /// @brief Move the cursor right @p count columns.
    void MoveCursorRight(int count = 1);
    /// @brief Move the cursor up @p count rows (returns to the prompt line
    /// after the completion menu is drawn below it).
    void MoveCursorUp(int count = 1);
    /// @brief Move the cursor down @p count rows (into the menu region).
    void MoveCursorDown(int count = 1);
    /// @brief Move the cursor to column @p col (0-based).
    void MoveCursorToCol(int col);
    /// @brief Erase from the cursor to the end of the current line.
    void ClearToEndOfLine();
    /// @brief Erase from the cursor to the end of the screen (wipes the menu).
    void ClearBelow();
    /// @brief Clear the whole screen and home the cursor.
    void ClearScreen();

    /// @brief Terminal width in columns (falls back to 80 if unknown).
    [[nodiscard]]
    int GetWidth();

    /// @brief Terminal height in rows (falls back to 24 if unknown).
    [[nodiscard]]
    int GetHeight();

private:
    KeyEvent ParseEscapeSequence();
    bool HasPendingInput(int timeoutMs);

    struct termios m_origTermios_;
    bool m_rawMode_;
};

} // namespace line
#endif // LINE_TERMINAL_HPP
