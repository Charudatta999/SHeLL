#ifndef EXEC_FD_CONTEXT_HPP
#define EXEC_FD_CONTEXT_HPP

#include "parser/ast/Redirect.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace io
{
class Pipe;
class FileDescriptor;
} // namespace io

namespace exec
{

class FdContext
{
public:
    FdContext();
    ~FdContext();
    FdContext(const FdContext&) = default;
    FdContext& operator=(const FdContext&) = default;
    FdContext(FdContext&&) = default;
    FdContext& operator=(FdContext&&) = default;

    std::unique_ptr<FdContext>
    ForStage(const std::vector<io::Pipe>& pipes,
             std::size_t idx,
             std::size_t nStages,
             const std::vector<parser::ast::Redirect>& redirects);

private:
    void SetIn(const std::unique_ptr<io::FileDescriptor>& fdIn);
    void SetOut(const std::unique_ptr<io::FileDescriptor>& fdOut);

    std::optional<std::reference_wrapper<io::FileDescriptor>> m_inFd_;
    std::optional<std::reference_wrapper<io::FileDescriptor>> m_outFd_;
    std::vector<parser::ast::Redirect> m_redirects_;
};

} // namespace exec
#endif // EXEC_FD_CONTEXT_HPP