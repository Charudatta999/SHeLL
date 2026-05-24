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
    if(oldFd.GetFD() == -1 && newFd == -1)
    {
        return false;
    }
    return dup2(oldFd.GetFD(), newFd) != -1;
}

std::unique_ptr<FileDescriptor> fdops::Dup(FileDescriptor& oldFd) noexcept
{
    int fd = -1;
    if(oldFd.GetFD() != -1)
    {
        fd = dup(oldFd.GetFD());
        if(fd != -1 )
        {
            return std::make_unique<FileDescriptor>(fd);
        }
    }
    return nullptr;
}

} // namespace io