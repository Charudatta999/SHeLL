#include "line/History.hpp"

#include <fstream>
#include <utility>

namespace line
{

History::History(std::string filePath, std::size_t maxEntries)
    : m_cursor_(0)
    , m_filePath_(std::move(filePath))
    , m_maxEntries_(maxEntries)
{
}

History::~History()
{
    Save();
}

void History::Load()
{
    std::ifstream file(m_filePath_);
    if (!file.is_open())
        return;

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty())
            m_entries_.push_back(line);
    }

    if (m_entries_.size() > m_maxEntries_)
        m_entries_.erase(m_entries_.begin(),
                         m_entries_.begin() +
                             static_cast<std::ptrdiff_t>(
                                 m_entries_.size() - m_maxEntries_));

    m_cursor_ = m_entries_.size();
}

void History::Save()
{
    if (m_filePath_.empty())
        return;

    std::ofstream file(m_filePath_, std::ios::trunc);
    if (!file.is_open())
        return;

    std::size_t start = 0;
    if (m_entries_.size() > m_maxEntries_)
        start = m_entries_.size() - m_maxEntries_;

    for (std::size_t idx = start; idx < m_entries_.size(); idx++)
        file << m_entries_[idx] << '\n';
}

void History::Add(const std::string& line)
{
    if (line.empty())
        return;

    if (!m_entries_.empty() && m_entries_.back() == line)
        return;

    m_entries_.push_back(line);

    if (m_entries_.size() > m_maxEntries_)
        m_entries_.erase(m_entries_.begin());

    m_cursor_ = m_entries_.size();
}

void History::Reset()
{
    m_cursor_ = m_entries_.size();
    m_savedLine_.clear();
}

const std::string& History::Up(const std::string& currentLine)
{
    if (m_entries_.empty())
        return currentLine;

    if (m_cursor_ == m_entries_.size())
        m_savedLine_ = currentLine;

    if (m_cursor_ > 0)
        m_cursor_--;

    return m_entries_[m_cursor_];
}

const std::string& History::Down(const std::string& currentLine)
{
    if (m_cursor_ >= m_entries_.size())
        return currentLine;

    m_cursor_++;

    if (m_cursor_ == m_entries_.size())
        return m_savedLine_;

    return m_entries_[m_cursor_];
}

bool History::AtEnd() const
{
    return m_cursor_ >= m_entries_.size();
}

std::size_t History::Size() const
{
    return m_entries_.size();
}

} // namespace line
