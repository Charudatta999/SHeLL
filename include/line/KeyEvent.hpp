#ifndef LINE_KEY_EVENT_HPP
#define LINE_KEY_EVENT_HPP

namespace line
{

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

struct KeyEvent
{
    Key key;
    char ch = '\0';
};

} // namespace line
#endif // LINE_KEY_EVENT_HPP
