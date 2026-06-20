#include "exec/FdContext.hpp"

#include "exec/FdContext.hpp"
#include "io/FdOps.hpp"
#include "io/FileDescriptor.hpp"

#include "io/Pipe.hpp"
#include <unistd.h>
#include <memory>

namespace exec
{
FdContext::FdContext() = default;
FdContext::~FdContext() = default;

std::unique_ptr<FdContext> FdContext::ForStage(
    const std::vector<io::Pipe>& pipes,
    std::size_t idx,
    std::size_t nStages,
    const std::vector<parser::ast::Redirect>& redirects)
{
    if(idx == 0)
    {
        SetOut(pipes[0].GetWritePipeFD());
        m_redirects_ = redirects;
        return std::make_unique<FdContext>(*this);
    }
    if (idx == nStages - 1)
    {
        SetIn(pipes[idx-1].GetReadPipeFD());
        m_redirects_ = redirects;
        return std::make_unique<FdContext>(*this);
    }
    SetIn(pipes[idx-1].GetReadPipeFD());
    SetOut(pipes[idx].GetWritePipeFD());
    m_redirects_ = redirects;
    return std::make_unique<FdContext>(*this);
}

} // namespace exec