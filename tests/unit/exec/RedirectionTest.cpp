// =========================================================
// RedirectionTest — ApplyRedirect rewires fds in the calling
// process, so successful cases run in a forked child (to not
// clobber the test runner's stdio); the parent inspects the
// file. Failure cases run in-process (open fails before any
// dup2, so fds stay intact).
// =========================================================

#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "exec/Redirection.hpp"
#include "parser/ast/Redirect.hpp"

namespace
{
using Redirect = parser::ast::Redirect;

// Unique temp path (created, then reused by the redirect's open()).
std::string tempPath()
{
    char templ[] = "/tmp/redir_XXXXXX";
    int fd = ::mkstemp(templ);
    ::close(fd);
    return templ;
}

std::string readFile(const std::string& path)
{
    std::ifstream in(path);
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

// Apply a redirect in a child, write `bytes` to `fd`, exit 0.
// Returns the child's exit code (2 = ApplyRedirect failed).
int childApplyAndWrite(const Redirect& redirect, int fd,
                       const std::string& bytes)
{
    pid_t pid = ::fork();
    if (pid == 0)
    {
        if (!exec::ApplyRedirect(redirect))
            _exit(2);
        ::write(fd, bytes.data(), bytes.size());
        _exit(0);
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
} // namespace

TEST(Redirection, OutTruncatesAndWritesStdout)
{
    std::string path = tempPath();
    Redirect r{.kind = Redirect::Kind::Out, .fd = -1, .target = path};

    EXPECT_EQ(childApplyAndWrite(r, 1, "hello"), 0);
    EXPECT_EQ(readFile(path), "hello");

    ::unlink(path.c_str());
}

TEST(Redirection, AppendAddsToExistingFile)
{
    std::string path = tempPath();
    { std::ofstream(path) << "a"; }

    Redirect r{.kind = Redirect::Kind::Append, .fd = -1, .target = path};
    EXPECT_EQ(childApplyAndWrite(r, 1, "b"), 0);
    EXPECT_EQ(readFile(path), "ab");

    ::unlink(path.c_str());
}

TEST(Redirection, ExplicitFdRedirectsStderr)
{
    std::string path = tempPath();
    // 2> file : redirect fd 2 to the file, write to fd 2.
    Redirect r{.kind = Redirect::Kind::Out, .fd = 2, .target = path};

    EXPECT_EQ(childApplyAndWrite(r, 2, "err"), 0);
    EXPECT_EQ(readFile(path), "err");

    ::unlink(path.c_str());
}

TEST(Redirection, InFeedsFileToStdin)
{
    std::string path = tempPath();
    { std::ofstream(path) << "input-data"; }

    pid_t pid = ::fork();
    if (pid == 0)
    {
        Redirect r{.kind = Redirect::Kind::In, .fd = -1, .target = path};
        if (!exec::ApplyRedirect(r))
            _exit(2);
        char buf[32] = {0};
        ssize_t n = ::read(0, buf, sizeof(buf) - 1);
        _exit(n == 10 ? 0 : 3); // "input-data" is 10 bytes
    }
    int status = 0;
    ::waitpid(pid, &status, 0);
    EXPECT_EQ(WEXITSTATUS(status), 0);

    ::unlink(path.c_str());
}

TEST(Redirection, OutToUnwritablePathFails)
{
    // open() fails before any dup2, so this is safe in-process.
    Redirect r{.kind = Redirect::Kind::Out,
               .fd = -1,
               .target = "/no_such_dir_xyz/file"};
    EXPECT_FALSE(exec::ApplyRedirect(r));
}

TEST(Redirection, InFromMissingFileFails)
{
    Redirect r{.kind = Redirect::Kind::In,
               .fd = -1,
               .target = "/tmp/definitely_missing_xyz_123"};
    EXPECT_FALSE(exec::ApplyRedirect(r));
}
