#include "io/FileDescriptor.hpp"

#include <unistd.h>

namespace io
{

FileDescriptor::FileDescriptor(int fd) noexcept
    : m_fd_(fd)
{
}

FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept
{
    this->m_fd_= other.m_fd_;
    other.m_fd_ = -1;
}

FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept
{
    if(this != &other)
    {
        this->Close();
        this->m_fd_= other.m_fd_;
        other.m_fd_ = -1;
    }
    return *this;
}

FileDescriptor::~FileDescriptor() noexcept
{
    if (m_fd_ != -1)
    {
        Close();
    }
}

bool FileDescriptor::Close()
{
    if (m_fd_ == -1)
    {
        return true;
    }
    close(m_fd_);
    m_fd_ = -1;
    return true;
}

int FileDescriptor::GetFD() const
{
    return m_fd_;
}
} // namespace io