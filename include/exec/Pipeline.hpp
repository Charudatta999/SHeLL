#ifndef EXEC_PIPELINE_HPP
#define EXEC_PIPELINE_HPP

#include <memory>
#include <string>
#include <vector>

namespace io
{
 class FileDescriptor;
 class Pipe;
}
namespace exec
{
struct CommandSpec;
class Pipeline
{

public:
    Pipeline();
    ~Pipeline() = default;

    // Non-Copyable & Non-Moveable
    Pipeline(const Pipeline& pipeline) = delete;
    Pipeline& operator= (const Pipeline& pipeline) = delete;
    Pipeline(Pipeline&& pipeline) = delete;
    Pipeline& operator= (Pipeline&& pipeline) = delete;

    int Run(const std::vector<CommandSpec>& pipeline, bool pipefail = false);
private:
    std::unique_ptr<io::Pipe> CreatePipe();
}; // class exec

} // namespace Pipeline
#endif // EXEC_PIPELINE_HPP