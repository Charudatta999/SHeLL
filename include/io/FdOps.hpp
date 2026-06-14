#ifndef IO_FD_OPS_HPP
#define IO_FD_OPS_HPP
#include <memory>

namespace io
{
// forward declare
class FileDescriptor;

namespace fdops
{
enum class ReadResult
{
    Ok,
    Eof,
    Interrupted,
    Error
};

bool ClearCloexec(FileDescriptor& fileDes) noexcept;
bool IsCloexecSet(FileDescriptor& fileDes) noexcept;
bool SetCloexec(FileDescriptor& fileDes) noexcept;

std::unique_ptr<FileDescriptor> Dup(FileDescriptor& oldFd) noexcept;
bool Dup2(FileDescriptor& oldFd, int newFd) noexcept;
bool WriteAll(int fileDes, std::string_view data);
ReadResult ReadByte(int fd, char& out);
} // namespace fdops
} // namespace io

#endif