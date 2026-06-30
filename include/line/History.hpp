#ifndef LINE_HISTORY_HPP
#define LINE_HISTORY_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace line
{

class History
{
public:
    explicit History(std::string filePath,
                     std::size_t maxEntries = 10000);
    ~History();
    History(const History&) = delete;
    History& operator=(const History&) = delete;
    History(History&&) = delete;
    History& operator=(History&&) = delete;

    void Load();
    void Save();
    void Add(const std::string& line);

    void Reset();
    [[nodiscard]]
    const std::string& Up(const std::string& currentLine);
    [[nodiscard]]
    const std::string& Down(const std::string& currentLine);
    [[nodiscard]]
    bool AtEnd() const;

    [[nodiscard]]
    std::size_t Size() const;

private:
    std::vector<std::string> m_entries_;
    std::size_t m_cursor_;
    std::string m_filePath_;
    std::size_t m_maxEntries_;
    std::string m_savedLine_;
};

} // namespace line
#endif // LINE_HISTORY_HPP
