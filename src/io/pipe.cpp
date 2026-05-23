// system headers
#include <array>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <utility>

// project headers
#include "io/IOException.hpp"
#include "io/Pipe.hpp"

namespace io
{

// constructor
// Note on pipe() vs pipe2():
// Standard pipe() creates file descriptors without the O_CLOEXEC flag. In a shell,
// this causes FDs to leak into every child process during fork/exec, which can lead
// to hanging processes because pipes won't signal EOF until all write-ends are closed.
//
// pipe2(fds, O_CLOEXEC) is highly recommended for Linux shells. It sets O_CLOEXEC
// atomically upon creation. FDs will close automatically during an exec(), preventing
// leaks. When linking processes (e.g. `ls | grep`), dup2() maps the pipe to stdin/stdout
// in the child prior to exec(), which safely strips O_CLOEXEC from the duplicated FD.
Pipe::Pipe()
    : m_readFD_(-1)
    , m_writeFD_(-1)
{
    std::array<int, 2> fds{-1, -1};
    if (pipe2(fds.data(), O_CLOEXEC) == 0)
    {
        m_readFD_ = FileDescriptor(0);
        m_writeFD_ = FileDescriptor(1);
    }
    else
    {
        throw IOException("Pipe creation failed", errno);
    }
}

Pipe::Pipe(Pipe&& other) noexcept
    : m_readFD_(std::exchange(other.m_readFD_, FileDescriptor(-1)))
    , m_writeFD_(std::exchange(other.m_writeFD_, FileDescriptor(-1)))
{
}

Pipe& Pipe::operator=(Pipe&& other) noexcept
{
    if (this != &other)
    {
        CloseReadFD();
        CloseWriteFD();
        this->m_readFD_ = std::exchange(other.m_readFD_, FileDescriptor(-1));
        this->m_writeFD_ = std::exchange(other.m_writeFD_, FileDescriptor(-1));
    }
    return *this;
}

// member functions
const FileDescriptor& Pipe::GetReadPipeFD() const noexcept
{
    return m_readFD_;
}

const FileDescriptor& Pipe::GetWritePipeFD() const noexcept
{
    return m_writeFD_;
}

void Pipe::CloseReadFD() noexcept
{
    if (m_readFD_.GetFD() != -1)
    {
        m_readFD_.Close();
    }
}

void Pipe::CloseWriteFD() noexcept
{
    if (m_writeFD_.GetFD() != -1)
    {
        m_writeFD_.Close();
    }
}

Pipe::~Pipe()
{
    CloseReadFD();
    CloseWriteFD();
}
} // namespace io