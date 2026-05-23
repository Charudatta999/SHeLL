#include "io/FileDescriptor.hpp"

#include <unistd.h>

namespace io
{

FileDescriptor::FileDescriptor(int fd) noexcept : fd_(fd)
{ 
}

FileDescriptor::~FileDescriptor() noexcept
{
    Close();
}

bool FileDescriptor::Close()
{
    if (close(fd_) == -1)
    {
        return false;
    }

    fd_ = -1;
    return true;
}

int FileDescriptor::GetFD()
{
    return fd_;
}
}