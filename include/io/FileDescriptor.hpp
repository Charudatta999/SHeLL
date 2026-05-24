#ifndef IO_FILE_DESCRIPTOR_HPP
#define IO_FILE_DESCRIPTOR_HPP
namespace io
{
class FileDescriptor final
{
private:
    int m_fd_;
public:
    FileDescriptor(const FileDescriptor& other) = delete;
    FileDescriptor& operator=(const FileDescriptor& other) = delete;
    explicit FileDescriptor(int fileDescriptor) noexcept;
    ~FileDescriptor() noexcept;

    FileDescriptor(FileDescriptor&& other) noexcept;
    FileDescriptor& operator=(FileDescriptor&& other) noexcept;

    bool Close();
    [[nodiscard]]
    int GetFD() const;
};
} // namespace io
#endif // IO_FILE_DESCRIPTOR_HPP