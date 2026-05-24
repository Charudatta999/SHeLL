#ifndef IO_PIPE_HPP
#define IO_PIPE_HPP

#include "io/FileDescriptor.hpp"

#include <memory>
namespace io
{

class Pipe
{
private:
    std::unique_ptr<FileDescriptor> m_readFD_;
    std::unique_ptr<FileDescriptor> m_writeFD_;

public:
    // constructors and destructor
    Pipe();
    ~Pipe();

    // Non-Copyable, Moveable
    Pipe(const Pipe& other) = delete;
    Pipe& operator=(const Pipe& other) = delete;
    Pipe(Pipe&& other) noexcept;
    Pipe& operator=(Pipe&& other) noexcept;

    // member functions
    [[nodiscard("output should not be ignored")]]
    const std::unique_ptr<FileDescriptor>& GetReadPipeFD() const noexcept;
    [[nodiscard("output should not be ignored")]]
    const std::unique_ptr<FileDescriptor>& GetWritePipeFD() const noexcept;

    // close FD's
    void CloseReadFD() noexcept;
    void CloseWriteFD() noexcept;

}; // Class Pipe
} // namespace io
#endif // IO_PIPE_HPP