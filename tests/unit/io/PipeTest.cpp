// system headers
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>
#include <array>

// project headers
#include "io/Pipe.hpp"

using namespace io;

// Basic construction and getters
TEST(PipeTest, ConstructorCreatesValidPipes)
{
    Pipe pipe0;
    // File descriptors 0, 1, and 2 are stdin, stdout, stderr.
    // Our pipe0 should always get FDs greater than 2.
    EXPECT_GT(pipe0.GetReadPipeFD(), 2);
    EXPECT_GT(pipe0.GetWritePipeFD(), 2);

    // Read and write FDs must be distinct
    EXPECT_NE(pipe0.GetReadPipeFD(), pipe0.GetWritePipeFD());
}

// Ensure closing FDs works and is safe to call multiple times (Idempotent)
TEST(PipeTest, CloseMethodsAreIdempotent)
{
    Pipe pipe0;
    int originalWriteFd = pipe0.GetWritePipeFD();

    // Close the read end
    pipe0.CloseReadFD();
    EXPECT_EQ(pipe0.GetReadPipeFD(), -1);

    // Calling it a second time shouldn't crash or trigger EBADF
    pipe0.CloseReadFD();
    EXPECT_EQ(pipe0.GetReadPipeFD(), -1);

    // The write end should still be completely unaffected
    EXPECT_EQ(pipe0.GetWritePipeFD(), originalWriteFd);

    // Close write end
    pipe0.CloseWriteFD();
    EXPECT_EQ(pipe0.GetWritePipeFD(), -1);
}

// Test our implementation of std::exchange in the Move Constructor
TEST(PipeTest, MoveConstructorTransfersOwnership)
{
    Pipe pipe1;
    int readFd = pipe1.GetReadPipeFD();
    int writeFd = pipe1.GetWritePipeFD();

    // Trigger the move constructor
    Pipe pipe2(std::move(pipe1));

    // pipe1 should now be completely empty
    EXPECT_EQ(pipe1.GetReadPipeFD(), -1);
    EXPECT_EQ(pipe1.GetWritePipeFD(), -1);

    // pipe2 should own the original FDs
    EXPECT_EQ(pipe2.GetReadPipeFD(), readFd);
    EXPECT_EQ(pipe2.GetWritePipeFD(), writeFd);
}

// Test the Move Assignment Operator and ensure old resources are cleaned up
TEST(PipeTest, MoveAssignmentTransfersOwnership)
{
    Pipe pipe1;
    int p1Read = pipe1.GetReadPipeFD();
    int p1Write = pipe1.GetWritePipeFD();

    Pipe pipe2; // pipe2 has its own FDs right now

    // Overwrite pipe2 with pipe1
    pipe2 = std::move(pipe1);

    // pipe1 should be empty
    EXPECT_EQ(pipe1.GetReadPipeFD(), -1);
    EXPECT_EQ(pipe1.GetWritePipeFD(), -1);

    // pipe2 should have stolen pipe1's FDs
    EXPECT_EQ(pipe2.GetReadPipeFD(), p1Read);
    EXPECT_EQ(pipe2.GetWritePipeFD(), p1Write);

    // Note: pipe2's original FDs were safely closed during the assignment
}

// End-to-end integration test: does it actually act like a pipe0?
TEST(PipeTest, DataTransmission)
{
    Pipe pipe0;
    const std::string testMsg = "Hello SHeLL!";

    // 1. Write data to the write end
    ssize_t bytesWritten = write(pipe0.GetWritePipeFD(), testMsg.c_str(), testMsg.size());
    EXPECT_EQ(bytesWritten, testMsg.size());

    // 2. Read data from the read end
    std::array<char, 128> buffer = {0};
    ssize_t bytesRead = read(pipe0.GetReadPipeFD(), buffer.data(), sizeof(buffer) - 1);

    // 3. Verify it traversed the pipe0 perfectly
    EXPECT_EQ(bytesRead, testMsg.size());
    EXPECT_STREQ(buffer.data(), testMsg.c_str());
}
