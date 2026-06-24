#include "line/LineEditor.hpp"

#include <unistd.h>

namespace line
{

LineEditor::LineEditor(Terminal& terminal, History& history)
    : m_terminal_(terminal)
    , m_history_(history)
{
}

std::optional<std::string> LineEditor::ReadLine(const std::string& prompt)
{
    m_buffer_.clear();
    m_cursor_ = 0;
    m_prompt_ = prompt;
    m_history_.Reset();

    write(STDOUT_FILENO, m_prompt_.c_str(), m_prompt_.size());

    while (true)
    {
        auto event = m_terminal_.ReadKey();

        switch (event.key)
        {
            case Key::Enter:
                write(STDOUT_FILENO, "\r\n", 2);
                return m_buffer_;

            case Key::CtrlD:
                if (m_buffer_.empty())
                    return std::nullopt;
                DeleteCharForward();
                break;

            case Key::CtrlC:
                Interrupt();
                break;

            case Key::Char:
                InsertChar(event.ch);
                break;

            case Key::Backspace:
                DeleteCharBackward();
                break;

            case Key::Delete:
                DeleteCharForward();
                break;

            case Key::Left:
            case Key::CtrlB:
                MoveCursorLeft();
                break;

            case Key::Right:
            case Key::CtrlF:
                MoveCursorRight();
                break;

            case Key::Home:
            case Key::CtrlA:
                MoveCursorHome();
                break;

            case Key::End:
            case Key::CtrlE:
                MoveCursorEnd();
                break;

            case Key::CtrlLeft:
                MoveWordLeft();
                break;

            case Key::CtrlRight:
                MoveWordRight();
                break;

            case Key::Up:
            case Key::CtrlP:
                HistoryUp();
                break;

            case Key::Down:
            case Key::CtrlN:
                HistoryDown();
                break;

            case Key::CtrlK:
                KillToEnd();
                break;

            case Key::CtrlU:
                KillToStart();
                break;

            case Key::CtrlW:
                KillWordBackward();
                break;

            case Key::CtrlY:
                Yank();
                break;

            case Key::CtrlL:
                ClearScreen();
                break;

            default:
                break;
        }
    }
}

void LineEditor::InsertChar(char chr)
{
    m_buffer_.insert(m_cursor_, 1, chr);
    m_cursor_++;
    Refresh();
}

void LineEditor::DeleteCharBackward()
{
    if (m_cursor_ == 0)
        return;
    m_buffer_.erase(m_cursor_ - 1, 1);
    m_cursor_--;
    Refresh();
}

void LineEditor::DeleteCharForward()
{
    if (m_cursor_ >= m_buffer_.size())
        return;
    m_buffer_.erase(m_cursor_, 1);
    Refresh();
}

void LineEditor::MoveCursorLeft()
{
    if (m_cursor_ > 0)
    {
        m_cursor_--;
        m_terminal_.MoveCursorLeft();
    }
}

void LineEditor::MoveCursorRight()
{
    if (m_cursor_ < m_buffer_.size())
    {
        m_cursor_++;
        m_terminal_.MoveCursorRight();
    }
}

void LineEditor::MoveCursorHome()
{
    m_cursor_ = 0;
    m_terminal_.MoveCursorToCol(static_cast<int>(m_prompt_.size()));
}

void LineEditor::MoveCursorEnd()
{
    m_cursor_ = m_buffer_.size();
    m_terminal_.MoveCursorToCol(
        static_cast<int>(m_prompt_.size() + m_buffer_.size()));
}

void LineEditor::MoveWordLeft()
{
    while (m_cursor_ > 0 && m_buffer_[m_cursor_ - 1] == ' ')
        m_cursor_--;
    while (m_cursor_ > 0 && m_buffer_[m_cursor_ - 1] != ' ')
        m_cursor_--;
    m_terminal_.MoveCursorToCol(
        static_cast<int>(m_prompt_.size() + m_cursor_));
}

void LineEditor::MoveWordRight()
{
    while (m_cursor_ < m_buffer_.size() && m_buffer_[m_cursor_] != ' ')
        m_cursor_++;
    while (m_cursor_ < m_buffer_.size() && m_buffer_[m_cursor_] == ' ')
        m_cursor_++;
    m_terminal_.MoveCursorToCol(
        static_cast<int>(m_prompt_.size() + m_cursor_));
}

void LineEditor::KillToEnd()
{
    m_killRing_ = m_buffer_.substr(m_cursor_);
    m_buffer_.erase(m_cursor_);
    Refresh();
}

void LineEditor::KillToStart()
{
    m_killRing_ = m_buffer_.substr(0, m_cursor_);
    m_buffer_.erase(0, m_cursor_);
    m_cursor_ = 0;
    Refresh();
}

void LineEditor::KillWordBackward()
{
    if (m_cursor_ == 0)
        return;

    std::size_t end = m_cursor_;
    while (m_cursor_ > 0 && m_buffer_[m_cursor_ - 1] == ' ')
        m_cursor_--;
    while (m_cursor_ > 0 && m_buffer_[m_cursor_ - 1] != ' ')
        m_cursor_--;

    m_killRing_ = m_buffer_.substr(m_cursor_, end - m_cursor_);
    m_buffer_.erase(m_cursor_, end - m_cursor_);
    Refresh();
}

void LineEditor::Yank()
{
    if (m_killRing_.empty())
        return;
    m_buffer_.insert(m_cursor_, m_killRing_);
    m_cursor_ += m_killRing_.size();
    Refresh();
}

void LineEditor::HistoryUp()
{
    const auto& line = m_history_.Up(m_buffer_);
    m_buffer_ = line;
    m_cursor_ = m_buffer_.size();
    Refresh();
}

void LineEditor::HistoryDown()
{
    const auto& line = m_history_.Down(m_buffer_);
    m_buffer_ = line;
    m_cursor_ = m_buffer_.size();
    Refresh();
}

void LineEditor::ClearScreen()
{
    m_terminal_.ClearScreen();
    write(STDOUT_FILENO, m_prompt_.c_str(), m_prompt_.size());
    write(STDOUT_FILENO, m_buffer_.c_str(), m_buffer_.size());
    m_terminal_.MoveCursorToCol(
        static_cast<int>(m_prompt_.size() + m_cursor_));
}

void LineEditor::Interrupt()
{
    write(STDOUT_FILENO, "^C\r\n", 4);
    m_buffer_.clear();
    m_cursor_ = 0;
    write(STDOUT_FILENO, m_prompt_.c_str(), m_prompt_.size());
}

void LineEditor::Refresh()
{
    write(STDOUT_FILENO, "\r", 1);
    write(STDOUT_FILENO, m_prompt_.c_str(), m_prompt_.size());
    write(STDOUT_FILENO, m_buffer_.c_str(), m_buffer_.size());
    m_terminal_.ClearToEndOfLine();
    m_terminal_.MoveCursorToCol(
        static_cast<int>(m_prompt_.size() + m_cursor_));
}

} // namespace line
