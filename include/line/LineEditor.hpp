#ifndef LINE_LINE_EDITOR_HPP
#define LINE_LINE_EDITOR_HPP


#include "line/CompletionPager.hpp"
#include "line/History.hpp"
#include "line/KeyEvent.hpp"
#include "line/Terminal.hpp"

#include <memory>
#include <optional>
#include <string>

namespace line
{
class Completer;
class LineEditor
{
public:
    LineEditor(Terminal& terminal, History& history, std::unique_ptr<Completer>& completer);
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
    /// @brief Handle Tab: query the completer and apply the result. One
    /// candidate is inserted in full; several extend the common prefix and
    /// open the completion menu.
    void TabComplete();

    /// @brief Route a key to the open menu. Returns true if consumed; false
    /// means "close the menu, keep the choice, then handle the key normally".
    bool HandlePagerKey(const KeyEvent& event);
    /// @brief Open the menu for a multi-candidate result.
    void OpenPager(const Result& result);
    /// @brief Close the menu and wipe the grid, keeping the current line.
    void ClosePager();
    /// @brief Put the highlighted candidate into the line.
    void PreviewSelection();
    /// @brief Put the originally typed word back into the line.
    void RestoreOriginalWord();
    /// @brief Redraw the prompt line and the menu grid below it.
    void RenderPager();

    void Refresh();

    Terminal& m_terminal_;
    History& m_history_;

    std::string m_buffer_;
    std::size_t m_cursor_ = 0;
    std::string m_killRing_;
    std::string m_prompt_;
    std::unique_ptr<Completer>& m_completer_;

    CompletionPager m_pager_;
    bool m_pagerActive_ = false;
    std::string m_pagerPrefix_;   ///< Line text before the completed word.
    std::string m_pagerSuffix_;   ///< Line text after the completed word.
    std::string m_pagerOrigWord_; ///< The word as typed, for Esc.
    std::string m_pagerFilter_;   ///< Filter text: candidates must start with this.
};

} // namespace line
#endif // LINE_LINE_EDITOR_HPP
