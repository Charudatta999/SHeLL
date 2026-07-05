#include "line/LineEditor.hpp"

#include "line/Completer.hpp"
#include "line/KeyEvent.hpp"

#include <unistd.h>
#include <vector>

namespace line
{

namespace
{

/// @brief The longest text prefix shared by every candidate.
std::string LongestCommonPrefix(const std::vector<Candidate>& items)
{
    if (items.empty())
        return {};
    std::string prefix = items.front().text;
    for (const auto& item : items)
    {
        std::size_t idx = 0;
        while (idx < prefix.size() && idx < item.text.size() &&
               prefix[idx] == item.text[idx])
            idx++;
        prefix.resize(idx);
    }
    return prefix;
}

} // namespace

LineEditor::LineEditor(Terminal& terminal,
                       History& history,
                       std::unique_ptr<Completer>& completer)
    : m_terminal_(terminal)
    , m_history_(history)
    , m_completer_(completer)
{
}

std::optional<std::string>
LineEditor::ReadLine(const std::string& prompt)
{
    m_buffer_.clear();
    m_cursor_ = 0;
    m_prompt_ = prompt;
    m_history_.Reset();

    write(STDOUT_FILENO, m_prompt_.c_str(), m_prompt_.size());

    while (true)
    {
        auto event = m_terminal_.ReadKey();

        if (m_pagerActive_)
        {
            if (HandlePagerKey(event))
                continue;
            // Not a menu key: accept the current choice, close the menu, and
            // fall through to handle the key as normal editing.
            ClosePager();
        }

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
            case Key::Tab:
                TabComplete();
                break;
            default:
                break;
        }
    }
}

void LineEditor::TabComplete()
{
    const Result result = m_completer_->Complete(m_buffer_, m_cursor_);
    if (result.candidates.empty())
        return;
    // Defensive: ignore a stale replace point.
    if (result.replaceStart > m_cursor_ || m_cursor_ > m_buffer_.size())
        return;

    if (result.candidates.size() == 1)
    {
        // A single hit finishes the word (trailing space unless it ends in
        // '/', so directories keep descending).
        std::string text = result.candidates.front().text;
        if (text.empty() || text.back() != '/')
            text += ' ';
        m_buffer_.replace(result.replaceStart, m_cursor_ - result.replaceStart,
                          text);
        m_cursor_ = result.replaceStart + text.size();
        Refresh();
        return;
    }

    // Several hits: extend the shared prefix, then open the menu.
    const std::string prefix = LongestCommonPrefix(result.candidates);
    m_buffer_.replace(result.replaceStart, m_cursor_ - result.replaceStart,
                      prefix);
    m_cursor_ = result.replaceStart + prefix.size();
    OpenPager(result);
}

void LineEditor::OpenPager(const Result& result)
{
    m_pager_.Open(result);
    if (m_pager_.Empty())
    {
        Refresh();
        return;
    }
    m_pagerActive_ = true;
    m_pagerPrefix_ = m_buffer_.substr(0, result.replaceStart);
    m_pagerSuffix_ = m_buffer_.substr(m_cursor_);
    m_pagerOrigWord_ =
        m_buffer_.substr(result.replaceStart, m_cursor_ - result.replaceStart);
    m_pagerFilter_ = m_pagerOrigWord_;
    PreviewSelection();
    RenderPager();
}

bool LineEditor::HandlePagerKey(const KeyEvent& event)
{
    switch (event.key)
    {
        case Key::Tab:
        case Key::Down:     m_pager_.Next(); break;
        case Key::ShiftTab:
        case Key::Up:       m_pager_.Prev(); break;
        case Key::Right:    m_pager_.NextColumn(); break;
        case Key::Left:     m_pager_.PrevColumn(); break;
        case Key::Enter:
            ClosePager(); // accept the choice; a second Enter runs the line
            return true;
        case Key::Escape:
        case Key::CtrlC:
            RestoreOriginalWord();
            ClosePager();
            return true;
        case Key::Char:
        {
            // Append char to filter and re-filter the candidates.
            m_pagerFilter_ += event.ch;
            m_pager_.Filter(m_pagerFilter_);
            if (m_pager_.Empty())
            {
                // No matches: undo the append and restore the filter.
                m_pagerFilter_.pop_back();
                m_pager_.Filter(m_pagerFilter_);
                return true; // consume the key, no sound
            }
            PreviewSelection();
            RenderPager();
            return true;
        }
        case Key::Backspace:
        {
            // Longer than the typed word: pop a char and widen the menu.
            // Back at the typed word: put it back and close the menu.
            if (m_pagerFilter_.size() > m_pagerOrigWord_.size())
            {
                m_pagerFilter_.pop_back();
                m_pager_.Filter(m_pagerFilter_);
                PreviewSelection();
                RenderPager();
                return true;
            }
            RestoreOriginalWord();
            ClosePager();
            return true;
        }
        default:
            return false; // caller closes the menu and handles the key
    }
    PreviewSelection();
    RenderPager();
    return true;
}

void LineEditor::PreviewSelection()
{
    if (m_pager_.Empty())
        return;
    const std::string& text = m_pager_.Selected().text;
    m_buffer_ = m_pagerPrefix_ + text + m_pagerSuffix_;
    m_cursor_ = m_pagerPrefix_.size() + text.size();
}

void LineEditor::RestoreOriginalWord()
{
    m_buffer_ = m_pagerPrefix_ + m_pagerOrigWord_ + m_pagerSuffix_;
    m_cursor_ = m_pagerPrefix_.size() + m_pagerOrigWord_.size();
}

void LineEditor::RenderPager()
{
    m_terminal_.MoveCursorToCol(0);
    write(STDOUT_FILENO, m_prompt_.data(), m_prompt_.size());
    write(STDOUT_FILENO, m_buffer_.data(), m_buffer_.size());
    m_terminal_.ClearToEndOfLine();
    m_terminal_.ClearBelow();
    m_pager_.Render(m_terminal_);
    m_terminal_.MoveCursorToCol(static_cast<int>(m_prompt_.size() + m_cursor_));
}

void LineEditor::ClosePager()
{
    m_pagerActive_ = false;
    m_terminal_.MoveCursorToCol(0);
    write(STDOUT_FILENO, m_prompt_.data(), m_prompt_.size());
    write(STDOUT_FILENO, m_buffer_.data(), m_buffer_.size());
    m_terminal_.ClearToEndOfLine();
    m_terminal_.ClearBelow();
    m_terminal_.MoveCursorToCol(static_cast<int>(m_prompt_.size() + m_cursor_));
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
    while (m_cursor_ < m_buffer_.size() &&
           m_buffer_[m_cursor_] != ' ')
        m_cursor_++;
    while (m_cursor_ < m_buffer_.size() &&
           m_buffer_[m_cursor_] == ' ')
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
