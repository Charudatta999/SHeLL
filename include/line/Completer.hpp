#ifndef LINE_COMPLETER_HPP
#define LINE_COMPLETER_HPP

#include <string>
#include <vector>

namespace line
{

/// @brief One completion entry: the text to insert plus a display label.
/// @details text replaces the token in the line; description is shown only in
/// the pager grid (right column) and may be empty for entries without one.
struct Candidate
{
    /// @brief Text inserted into the line when this candidate is chosen.
    std::string text;
    /// @brief Human-readable label shown in the pager (may be empty).
    std::string description;
};

/// @brief Outcome of one completion query.
/// @details The editor replaces buffer[replaceStart, cursor) with a candidate
/// (or their common prefix). Empty candidates means "nothing to complete".
struct Result
{
    /// @brief Candidate entries for the token being completed.
    std::vector<Candidate> candidates;
    /// @brief Buffer index where candidates begin replacing.
    std::size_t replaceStart = 0;
};

/// @brief Turns a line + cursor into completion candidates.
/// @details Generic seam so LineEditor stays shell-agnostic: it hands over the
/// raw buffer and never learns command-vs-path. Implemented by ShellCompleter
/// (PATH, builtins, filesystem); can be faked in tests.
class Completer
{
public:
    Completer() = default;
    virtual ~Completer() = default;
    Completer(const Completer&) = delete;
    Completer& operator=(const Completer&) = delete;
    Completer(Completer&&) = delete;
    Completer& operator=(Completer&&) = delete;

    /// @brief Compute completions for the token ending at the cursor.
    /// @param line The full input buffer.
    /// @param cursor Byte index of the cursor within line.
    /// @return Candidates and the index where they replace; empty if none.
    [[nodiscard]]
    virtual Result Complete(const std::string& line,
                            std::size_t cursor) const = 0;

};

} // namespace line

#endif // LINE_COMPLETER_HPP
