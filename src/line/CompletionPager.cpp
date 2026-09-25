#include "line/CompletionPager.hpp"

#include "line/Terminal.hpp"

#include <algorithm>
#include <string>
#include <unistd.h>

namespace line
{

namespace
{
constexpr std::size_t kGap = 2;    // space between text and its description
constexpr std::size_t kColGap = 2; // space between columns

void WriteStr(const std::string& text)
{
    write(STDOUT_FILENO, text.data(), text.size());
}

/// @brief How a description is shown in the grid: fish wraps it in parens.
std::string DisplayDesc(const std::string& desc)
{
    return desc.empty() ? std::string{} : "(" + desc + ")";
}
} // namespace

void CompletionPager::Open(const Result& result)
{
    m_all_ = result.candidates;
    m_items_ = m_all_;
    m_replaceStart_ = result.replaceStart;
    m_selected_ = 0;
    m_rows_ = 1;
    m_filter_.clear();
}

void CompletionPager::Render(Terminal& terminal)
{
    if (m_items_.empty())
        return;

    // Cell width = widest text + gap + widest description.
    std::size_t textWidth = 0;
    std::size_t descWidth = 0;
    for (const auto& item : m_items_)
    {
        textWidth = std::max(textWidth, item.text.size());
        descWidth = std::max(descWidth, DisplayDesc(item.description).size());
    }
    const std::size_t cellWidth = textWidth + kGap + descWidth;

    const auto width = static_cast<std::size_t>(std::max(terminal.GetWidth(), 1));
    const std::size_t columns =
        std::max<std::size_t>(1, (width + kColGap) / (cellWidth + kColGap));

    const std::size_t count = m_items_.size();
    // Cap the menu to about half the screen (like fish) so it does not swallow
    // the whole terminal; the rest overflows into "...and N more".
    const auto maxRows =
        static_cast<std::size_t>(std::max((terminal.GetHeight() - 1) / 2, 1));

    // Rows to hold everything with all visible columns, capped to the height.
    std::size_t rows = (count + columns - 1) / columns; // ceil
    if (rows > maxRows)
        rows = maxRows;
    m_rows_ = rows;

    const std::size_t totalCols = (count + rows - 1) / rows; // ceil

    // Scroll horizontally so the page holding the selection is the one shown.
    const std::size_t selectedCol = m_selected_ / rows;
    const std::size_t firstCol = (selectedCol / columns) * columns;
    const std::size_t lastCol = std::min(totalCols, firstCol + columns);

    std::string out;
    for (std::size_t row = 0; row < rows; ++row)
    {
        out += "\r\n";
        for (std::size_t col = firstCol; col < lastCol; ++col)
        {
            const std::size_t idx = (col * rows) + row; // column-major
            if (idx >= count)
                continue; // empty slot at the tail of the last column
            const Candidate& item = m_items_[idx];
            std::string cell = item.text;
            cell.resize(textWidth + kGap, ' ');
            cell += DisplayDesc(item.description);
            cell.resize(cellWidth, ' ');

            if (idx == m_selected_)
                out += "\x1b[7m" + cell + "\x1b[0m"; // reverse video
            else
                out += cell;
            out += std::string(kColGap, ' ');
        }
        out += "\x1b[K"; // clear the rest of the row
    }

    std::size_t linesDrawn = rows;
    const std::size_t shownThrough = std::min(count, lastCol * rows);
    const std::size_t more = count - shownThrough;
    if (more > 0)
    {
        out += "\r\n...and " + std::to_string(more) + " more\x1b[K";
        linesDrawn += 1;
    }

    WriteStr(out);
    // Return the cursor to the prompt line (the caller repositions the column).
    terminal.MoveCursorUp(static_cast<int>(linesDrawn));
    WriteStr("\r");
}

void CompletionPager::Clear(Terminal& terminal)
{
    terminal.ClearBelow();
}

void CompletionPager::Next()
{
    if (m_items_.empty())
        return;
    m_selected_ = (m_selected_ + 1) % m_items_.size();
}

void CompletionPager::Prev()
{
    if (m_items_.empty())
        return;
    m_selected_ = (m_selected_ + m_items_.size() - 1) % m_items_.size();
}

void CompletionPager::NextColumn()
{
    if (m_items_.empty())
        return;
    m_selected_ = (m_selected_ + m_rows_) % m_items_.size();
}

void CompletionPager::PrevColumn()
{
    if (m_items_.empty())
        return;
    const std::size_t step = m_rows_ % m_items_.size();
    m_selected_ = (m_selected_ + m_items_.size() - step) % m_items_.size();
}

void CompletionPager::Filter(const std::string& text)
{
    m_filter_ = text;
    m_items_.clear();
    for (const auto& item : m_all_)
        if (item.text.starts_with(text))
            m_items_.push_back(item);
    m_selected_ = 0;
}

const Candidate& CompletionPager::Selected() const
{
    return m_items_.at(m_selected_);
}

std::size_t CompletionPager::ReplaceStart() const
{
    return m_replaceStart_;
}

bool CompletionPager::Empty() const
{
    return m_items_.empty();
}

} // namespace line
