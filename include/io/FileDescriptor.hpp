#ifndef IO_FILE_DESCRIPTOR_HPP
#define IO_FILE_DESCRIPTOR_HPP
namespace io
{
class FileDescriptor final
{
private:
    int fd_;

    FileDescriptor(const FileDescriptor& other) = delete;
    FileDescriptor& operator=(const FileDescriptor& other) = delete;

public:
    explicit FileDescriptor(int fd) noexcept;
    ~FileDescriptor() noexcept;

    FileDescriptor(FileDescriptor&& other) noexcept;
    FileDescriptor& operator=(FileDescriptor&& other) noexcept;

    bool Close();
    [[nodiscard]]
    int GetFD() const;
};
} // namespace io
#endif // IO_FILE_DESCRIPTOR_HPP