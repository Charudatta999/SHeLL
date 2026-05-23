#include "io/FileDescriptor.hpp"

#include <unistd.h>

namespace io
{

FileDescriptor::FileDescriptor(int fd) noexcept : fd_(fd)
{
}

FileDescriptor::~FileDescriptor() noexcept
{
    if (fd_ != -1)
    {
        Close();
    }
}

bool FileDescriptor::Close()
{
    if (fd_ == -1)
    {
        return true;
    }
    close(fd_);
    fd_ = -1;
    return true;
}

int FileDescriptor::GetFD() const
{
    return fd_;
}
}