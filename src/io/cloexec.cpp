#include "io/CloExec.hpp"

#include "io/FileDescriptor.hpp"

#include <fcntl.h>
#include <unistd.h>

namespace io
{

bool cloexec::Clear(FileDescriptor& fileDes) noexcept
{
    auto fdFlags = fcntl(fileDes.GetFD(), F_GETFD);
    if (fdFlags == -1)
    {
        return false;
    }
    fdFlags = fdFlags & (~FD_CLOEXEC);
    return fcntl(fileDes.GetFD(), F_SETFD, fdFlags) != -1;
}

bool cloexec::IsSet(FileDescriptor& fileDes) noexcept
{
    auto fdFlags = fcntl(fileDes.GetFD(), F_GETFD);
    if (fdFlags == -1)
    {
        return false;
    }
    return (fdFlags & FD_CLOEXEC) <= 0;
}

bool cloexec::Set(FileDescriptor &fileDes) noexcept
{
    auto fdFlags = fcntl(fileDes.GetFD(), F_GETFD);
    fdFlags = fdFlags & (FD_CLOEXEC);
    return  fcntl(fileDes.GetFD(), F_SETFD, fdFlags) != -1;
}

} // namespace io