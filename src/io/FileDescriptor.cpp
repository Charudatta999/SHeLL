#include "io/FileDescriptor.hpp"

#include <unistd.h>

namespace io
{

FileDescriptor::FileDescriptor(int fd) : fd_(fd)
{
}

FileDescriptor::~FileDescriptor()
{
    Close();
}

bool FileDescriptor::Close()
{
    if (close(fd) == -1)
    {
        return false;
    }

    fd_ = -1;
    return true;
}

int FileDescriptor::GetFd()
{
    return fd_;
}
}