#ifndef EXEC_PIPELINE_HPP
#define EXEC_PIPELINE_HPP

#include "exec/WaitStatus.hpp"

#include <cstdint>
#include <memory>
#include <sys/types.h>
#include <vector>

namespace io
{
class FileDescriptor;
class Pipe;
} // namespace io

namespace exec
{
struct CommandSpec;

// The two outcomes a synchronous Run can truthfully report: the job
// finished (Done) or a signal froze it (Stopped) — never Running,
// because Run always waits before returning.
enum class State : std::uint8_t
{
    Stopped,
    Done,
};

struct PipelineResult
{
    int status;
    pid_t pgid;
    State state;
};

class Pipeline
{

public:
    Pipeline();
    ~Pipeline() = default;

    // Non-Copyable & Non-Moveable
    Pipeline(const Pipeline& pipeline) = delete;
    Pipeline& operator=(const Pipeline& pipeline) = delete;
    Pipeline(Pipeline&& pipeline) = delete;
    Pipeline& operator=(Pipeline&& pipeline) = delete;

    PipelineResult Run(const std::vector<CommandSpec>& pipeline,
            bool pipefail = false,
            WaitMode mode = WaitMode::Foreground);

private:
    std::unique_ptr<io::Pipe> CreatePipe();
}; // class exec

} // namespace exec
#endif // EXEC_PIPELINE_HPP