#ifndef LINE_KEY_EVENT_HPP
#define LINE_KEY_EVENT_HPP

namespace line
{

/// @brief A decoded key press, independent of its terminal byte sequence.
enum class Key
{
    Char,

    // Navigation
    Up,
    Down,
    Left,
    Right,
    Home,
    End,
    CtrlLeft,
    CtrlRight,

    // Editing
    Backspace,
    Delete,
    Enter,
    Tab,
    ShiftTab, ///< Back-tab (CSI Z); reverses completion cycling.

    // Ctrl combos
    CtrlA,
    CtrlB,
    CtrlC,
    CtrlD,
    CtrlE,
    CtrlF,
    CtrlK,
    CtrlL,
    CtrlN,
    CtrlP,
    CtrlR,
    CtrlU,
    CtrlW,
    CtrlY,

    Escape,
    Unknown,
};

/// @brief A key press plus, for @ref Key::Char, the literal character.
struct KeyEvent
{
    Key key = Key::Unknown;
    char ch = '\0';
};

} // namespace line
#endif // LINE_KEY_EVENT_HPP
