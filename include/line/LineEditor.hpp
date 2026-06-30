#ifndef LINE_LINE_EDITOR_HPP
#define LINE_LINE_EDITOR_HPP

#include "line/History.hpp"
#include "line/KeyEvent.hpp"
#include "line/Terminal.hpp"

#include <optional>
#include <string>

namespace line
{

class LineEditor
{
public:
    LineEditor(Terminal& terminal, History& history);
    ~LineEditor() = default;
    LineEditor(const LineEditor&) = delete;
    LineEditor& operator=(const LineEditor&) = delete;
    LineEditor(LineEditor&&) = delete;
    LineEditor& operator=(LineEditor&&) = delete;

    [[nodiscard]]
    std::optional<std::string> ReadLine(const std::string& prompt);

private:
    void InsertChar(char chr);
    void DeleteCharBackward();
    void DeleteCharForward();
    void MoveCursorLeft();
    void MoveCursorRight();
    void MoveCursorHome();
    void MoveCursorEnd();
    void MoveWordLeft();
    void MoveWordRight();
    void KillToEnd();
    void KillToStart();
    void KillWordBackward();
    void Yank();
    void HistoryUp();
    void HistoryDown();
    void ClearScreen();
    void Interrupt();

    void Refresh();

    Terminal& m_terminal_;
    History& m_history_;

    std::string m_buffer_;
    std::size_t m_cursor_ = 0;
    std::string m_killRing_;
    std::string m_prompt_;
};

} // namespace line
#endif // LINE_LINE_EDITOR_HPP
