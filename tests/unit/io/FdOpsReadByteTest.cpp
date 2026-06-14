// =========================================================
// FdOpsReadByteTest — one-byte read with result classification
// (Ok / Eof / Error). Uses a pipe.
// =========================================================

#include <gtest/gtest.h>

#include <string>
#include <unistd.h>

#include "io/FdOps.hpp"

TEST(FdOpsReadByte, OkReturnsByte)
{
    int fds[2];
    ASSERT_EQ(::pipe(fds), 0);
    ASSERT_EQ(::write(fds[1], "x", 1), 1);

    char chr = 0;
    EXPECT_EQ(io::fdops::ReadByte(fds[0], chr), io::fdops::ReadResult::Ok);
    EXPECT_EQ(chr, 'x');

    ::close(fds[0]);
    ::close(fds[1]);
}

TEST(FdOpsReadByte, EofWhenWriteEndClosed)
{
    int fds[2];
    ASSERT_EQ(::pipe(fds), 0);
    ::close(fds[1]); // no writers -> read sees EOF

    char chr = 0;
    EXPECT_EQ(io::fdops::ReadByte(fds[0], chr), io::fdops::ReadResult::Eof);

    ::close(fds[0]);
}

TEST(FdOpsReadByte, ErrorOnBadFd)
{
    char chr = 0;
    EXPECT_EQ(io::fdops::ReadByte(-1, chr), io::fdops::ReadResult::Error);
}

// ─── WriteAll ────────────────────────────────────────────────────────────────
TEST(FdOpsWriteAll, WritesEveryByte)
{
    int fds[2];
    ASSERT_EQ(::pipe(fds), 0);

    const std::string payload = "hello world";
    EXPECT_TRUE(io::fdops::WriteAll(fds[1], payload));

    std::string got(payload.size(), '\0');
    ASSERT_EQ(::read(fds[0], got.data(), got.size()),
              static_cast<ssize_t>(payload.size()));
    EXPECT_EQ(got, payload);

    ::close(fds[0]);
    ::close(fds[1]);
}

TEST(FdOpsWriteAll, EmptyDataSucceeds)
{
    int fds[2];
    ASSERT_EQ(::pipe(fds), 0);
    EXPECT_TRUE(io::fdops::WriteAll(fds[1], ""));
    ::close(fds[0]);
    ::close(fds[1]);
}

TEST(FdOpsWriteAll, FailsOnBadFd)
{
    EXPECT_FALSE(io::fdops::WriteAll(-1, "x"));
}
