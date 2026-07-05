
#include "shell/ShellCompleter.hpp"

#include <algorithm>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>
#include <utility>
#include <vector>

namespace shell
{

namespace
{

/// @brief True if @p path is itself a symlink (does not follow it).
bool IsSymlink(const std::string& path)
{
    struct stat info{};
    return lstat(path.c_str(), &info) == 0 && S_ISLNK(info.st_mode);
}

/// @brief Build a file candidate: the basename (directories get a trailing
/// '/') plus a type label. stat follows symlinks, so a symlink to a
/// directory shows up as a directory.
line::Candidate MakeFileCandidate(std::string name, const std::string& fullPath)
{
    std::string description = "file";
    struct stat info{};
    if (stat(fullPath.c_str(), &info) == 0)
    {
        switch (info.st_mode & S_IFMT)
        {
            case S_IFDIR:
                name += '/';
                description = "directory";
                break;
            case S_IFREG:
                description = (info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
                                  ? "executable"
                                  : "file";
                break;
            case S_IFCHR: description = "char device"; break;
            case S_IFBLK: description = "block device"; break;
            case S_IFIFO: description = "fifo"; break;
            case S_IFSOCK: description = "socket"; break;
            default: break;
        }
    }
    return {.text = std::move(name), .description = std::move(description)};
}

/// @brief Append entries of `dirPath` that start with `base` to `out`.
/// Skips "." / ".." and hidden files unless `base` itself starts with a dot.
void CollectFiles(const std::string& dirPath, const std::string& base,
                  std::vector<line::Candidate>& out)
{
    DIR* dir = opendir(dirPath.c_str());
    if (dir == nullptr)
        return;
    const dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr)
    {
        const std::string name(entry->d_name);
        if (name == "." || name == "..")
            continue;
        if (name.front() == '.' && (base.empty() || base.front() != '.'))
            continue;
        if (!name.starts_with(base))
            continue;

        std::string full = dirPath;
        if (full.back() != '/')
            full += '/';
        full += name;
        out.push_back(MakeFileCandidate(name, full));
    }
    closedir(dir);
}

/// @brief Append executables in `dirPath` that start with `word` to `out`.
/// `seen` dedups across PATH directories (first directory wins).
void CollectExecutables(const std::string& dirPath, const std::string& word,
                        std::unordered_set<std::string>& seen,
                        std::vector<line::Candidate>& out)
{
    DIR* dir = opendir(dirPath.c_str());
    if (dir == nullptr)
        return;
    const dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr)
    {
        const std::string name(entry->d_name);
        if (name == "." || name == ".." || !name.starts_with(word))
            continue;

        std::string full = dirPath;
        if (full.back() != '/')
            full += '/';
        full += name;
        if (access(full.c_str(), X_OK) != 0)
            continue;
        if (!seen.insert(name).second)
            continue;
        const bool link = entry->d_type == DT_LNK ||
                          (entry->d_type == DT_UNKNOWN && IsSymlink(full));
        out.push_back({.text = name,
                       .description = link ? "command link" : "command"});
    }
    closedir(dir);
}

} // namespace

ShellCompleter::ShellCompleter(
    std::unique_ptr<ShellState>& state,
    std::unique_ptr<builtins::BuiltinDispatcher>& dispatcher)
    : m_state_(state)
    , m_dispatcher_(dispatcher)
{
}

line::Result ShellCompleter::Complete(const std::string& line,
                                      std::size_t cursor) const
{
    // Keep the cursor inside the line so the scans below cannot run off
    // the end.
    cursor = std::min(cursor, line.size());

    // Step 1: find where the word under the cursor starts. Walk left to
    // the previous space; line[start, cursor) is the word we complete.
    std::size_t start = cursor;
    while (start > 0 && line[start - 1] != ' ')
        start--;
    const std::string word = line.substr(start, cursor - start);

    // Step 2: command or argument? Skip spaces left of the word, then
    // look at the char before it. Start of line or a separator (| ; &)
    // means a command starts here; anything else means an argument.
    std::size_t scan = start;
    while (scan > 0 && line[scan - 1] == ' ')
        scan--;
    const bool isCommand = scan == 0 || line[scan - 1] == '|' ||
                           line[scan - 1] == ';' || line[scan - 1] == '&';

    // Step 3: hand off to the matching helper.
    line::Result result =
        isCommand ? CompleteCommand(word, start) : CompleteFile(word, start);

    // Step 4: sort for a stable, readable menu.
    std::ranges::sort(result.candidates, {}, &line::Candidate::text);
    return result;
}

line::Result ShellCompleter::CompleteCommand(const std::string& word,
                                             std::size_t start) const
{
    std::vector<line::Candidate> candidates;
    std::unordered_set<std::string> seen;

    // Builtins, with their descriptions. They shadow PATH, so remember
    // their names to skip a duplicate executable of the same name.
    for (const auto& name : m_dispatcher_->Names())
        if (name.starts_with(word))
        {
            candidates.push_back(
                {.text = name, .description = m_dispatcher_->Description(name)});
            seen.insert(name);
        }

    // Executables on PATH: split on ':', scan each directory.
    const std::string pathVar = m_state_->GetVar("PATH").value_or("");
    std::size_t begin = 0;
    while (true)
    {
        const std::size_t colon = pathVar.find(':', begin);
        const std::string dirPath = pathVar.substr(
            begin, colon == std::string::npos ? std::string::npos
                                              : colon - begin);
        if (!dirPath.empty())
            CollectExecutables(dirPath, word, seen, candidates);
        if (colon == std::string::npos)
            break;
        begin = colon + 1;
    }

    return line::Result{.candidates = std::move(candidates),
                        .replaceStart = start};
}

line::Result ShellCompleter::CompleteFile(const std::string& word,
                                          std::size_t start) const
{
    std::vector<line::Candidate> candidates;
    std::size_t replaceStart = start;

    // Split the word on its last '/': the part before is the directory to
    // read, the part after is the prefix. Candidates are basenames, so
    // replaceStart moves past the slash.
    const std::size_t slash = word.find_last_of('/');
    std::string dirPath = m_state_->GetCWD();
    std::string base = word;
    if (slash != std::string::npos)
    {
        dirPath = word.substr(0, slash + 1); // keep the slash: "/home/"
        base = word.substr(slash + 1);
        replaceStart = start + slash + 1;
        // A relative path is relative to the shell's CWD, not to whatever
        // directory this process happens to run in.
        if (dirPath.front() != '/')
            dirPath = m_state_->GetCWD() + '/' + dirPath;
    }

    CollectFiles(dirPath, base, candidates);

    return line::Result{.candidates = std::move(candidates),
                        .replaceStart = replaceStart};
}

} // namespace shell
