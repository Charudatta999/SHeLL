#ifndef IO_CLOEXEC_HPP
#define IO_CLOEXEC_HPP

namespace io
{
// forward declare
class FileDescriptor;

namespace cloexec
{

bool Clear(FileDescriptor& fileDes) noexcept;
bool IsSet(FileDescriptor& fileDes) noexcept;
bool Set(FileDescriptor& fileDes) noexcept;

} // namespace cloexec

} // namespace io

#endif