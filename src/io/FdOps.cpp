#include "io/FdOps.hpp"

#include "io/FileDescriptor.hpp"

#include <fcntl.h>
#include <memory>
#include <unistd.h>

namespace io
{

bool fdops::ClearCloexec(FileDescriptor& fileDes) noexcept
{
    auto fdFlags = fcntl(fileDes.GetFD(), F_GETFD);
    if (fdFlags == -1)
    {
        return false;
    }
    fdFlags = fdFlags & (~FD_CLOEXEC);
    return fcntl(fileDes.GetFD(), F_SETFD, fdFlags) != -1;
}

bool fdops::IsCloexecSet(FileDescriptor& fileDes) noexcept
{
    auto fdFlags = fcntl(fileDes.GetFD(), F_GETFD);
    if (fdFlags == -1)
    {
        return false;
    }
    return (fdFlags & FD_CLOEXEC) <= 0;
}

bool fdops::SetCloexec(FileDescriptor& fileDes) noexcept
{
    auto fdFlags = fcntl(fileDes.GetFD(), F_GETFD);
    fdFlags = fdFlags & (FD_CLOEXEC);
    return fcntl(fileDes.GetFD(), F_SETFD, fdFlags) != -1;
}

bool fdops::Dup2(FileDescriptor& oldFd, int newFd) noexcept
{
    if (oldFd.GetFD() == -1 && newFd == -1)
    {
        return false;
    }
    return dup2(oldFd.GetFD(), newFd) != -1;
}

std::unique_ptr<FileDescriptor>
fdops::Dup(FileDescriptor& oldFd) noexcept
{
    int fd = -1;
    if (oldFd.GetFD() != -1)
    {
        fd = dup(oldFd.GetFD());
        if (fd != -1)
        {
            return std::make_unique<FileDescriptor>(fd);
        }
    }
    return nullptr;
}

bool fdops::WriteAll(int fileDes, std::string_view data)
{
    std::size_t sent = 0;
    while (sent < data.size())
    {
        ssize_t written =
            write(fileDes, data.data() + sent, data.size() - sent);
        if (written == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return false;
        }
        sent += static_cast<std::size_t>(written);
    }
    return true;
}

fdops::ReadResult fdops::ReadByte(int fd, char& out)
{
    ssize_t res = read(fd, &out, 1);
    switch (res)
    {
        case 1:
        {
            return fdops::ReadResult::Ok;
        }
        case 0:
        {
            return fdops::ReadResult::Eof;
        }
        case -1:
        {
            if (errno == EINTR)
            {
                return fdops::ReadResult::Interrupted;
            }
            return fdops::ReadResult::Error;
        }
        default:
            return fdops::ReadResult::Error;
    }
}
} // namespace io