#ifndef LINE_COMPLETION_PAGER_HPP
#define LINE_COMPLETION_PAGER_HPP

#include "line/Completer.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace line
{

class Terminal;

/// @brief The completion menu: a grid of candidates below the prompt with a
/// moving highlight, descriptions, live filtering, and pagination.
class CompletionPager
{
public:
    CompletionPager() = default;
    ~CompletionPager() = default;
    CompletionPager(const CompletionPager&) = delete;
    CompletionPager& operator=(const CompletionPager&) = delete;
    CompletionPager(CompletionPager&&) = delete;
    CompletionPager& operator=(CompletionPager&&) = delete;

    /// @brief Load candidates and the replace point; reset the selection.
    void Open(const Result& result);

    /// @brief Draw the grid below the prompt, highlighting the selection.
    void Render(Terminal& terminal);

    /// @brief Erase the grid and return the cursor to the prompt line.
    void Clear(Terminal& terminal);

    /// @brief Move the highlight one cell forward / back (wrapping).
    void Next();
    void Prev();

    /// @brief Move the highlight one column right / left.
    void NextColumn();
    void PrevColumn();

    /// @brief Keep only candidates that still match `text`.
    void Filter(const std::string& text);

    /// @brief The currently highlighted candidate. Caller must check Empty().
    [[nodiscard]] const Candidate& Selected() const;

    /// @brief Buffer index where the chosen text replaces.
    [[nodiscard]] std::size_t ReplaceStart() const;

    /// @brief True when there is nothing to show.
    [[nodiscard]] bool Empty() const;

private:
    std::vector<Candidate> m_all_;   ///< All candidates (unfiltered source).
    std::vector<Candidate> m_items_; ///< Currently visible candidates.
    std::size_t m_replaceStart_ = 0;
    std::size_t m_selected_ = 0;
    std::size_t m_rows_ = 1;         ///< Grid rows from the last Render.
    std::string m_filter_;
};

} // namespace line
#endif // LINE_COMPLETION_PAGER_HPP
