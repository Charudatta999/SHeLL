#include "exec/Pipeline.hpp"
#include "exec/ExecException.hpp"
#include "exec/ExecHelpers.hpp"
#include "exec/Process.hpp"
#include "exec/WaitStatus.hpp"
#include "io/FdOps.hpp"
#include "io/IOException.hpp"
#include "io/Pipe.hpp"
#include "utils/ErrorCodes.hpp"

#include <cerrno>
#include <cstddef>
#include <memory>
#include <sched.h>
#include <sys/syslog.h>
#include <syslog.h>

namespace exec
{

Pipeline::Pipeline() = default;

std::unique_ptr<io::Pipe> Pipeline::CreatePipe()
{
    try
    {
        return std::make_unique<io::Pipe>();
    }
    catch (const io::IOException& ex)
    {
        syslog(LOG_ERR,
               "SHELL [exec] [Pipeline]: Failed to create pipe: %s (errno=%d)",
               ex.what(),
               ex.GetErrorCode());
        throw ExecException("failed To create pipe", FAILED_TO_CREATE);
    }
}

int Pipeline::Run(const std::vector<CommandSpec>& pipeline, bool pipefail)
{
    std::vector<std::unique_ptr<io::Pipe>> pipeVector;
    const size_t pipeSize = pipeline.size();
    if (pipeSize == 0) return 0;
    try
    {
        for (size_t i = 0; i < pipeSize - 1; i++)
        {
            pipeVector.push_back(CreatePipe());
        }
    }
    catch (const ExecException& ex)
    {
        auto error = std::string("SHELL [exec] [Pipeline] : Failed to run command: ") +
                     std::string(ex.what()) + " with errno: " + std::to_string(errno);
        throw ExecException(error, ex.GetErrorCode());
    }
    std::vector<std::unique_ptr<Process>> processVector(pipeSize);
    std::vector<std::unique_ptr<WaitStatus>> statusVector(pipeSize);
    if (pipeSize == 1)
    {
        Process p;
        p.Start(pipeline[0], nullptr, nullptr,0);
        WaitStatus s(p.GetPid());
        if (s.Signaled()) return s.GetSignal();
        if (s.Exited()) return s.ExitCode();
        return INVALID_STATUS;
    }
    for (size_t i = 0; i < pipeSize; i++)
    {
        processVector[i] = std::make_unique<Process>();
        if (i == 0)
        {
            processVector[i]->Start(pipeline[i], nullptr, pipeVector[i]->GetWritePipeFD(),0);
        }
        else if (i > 0 && i < pipeSize - 1)
        {
            auto pgid = processVector[0]->GetPid();
            processVector[i]->Start(
                pipeline[i], pipeVector[i - 1]->GetReadPipeFD(), pipeVector[i]->GetWritePipeFD(), pgid);
        }
        else if (i == pipeSize - 1)
        {
            auto pgid = processVector[0]->GetPid();
            processVector[i]->Start(pipeline[i], pipeVector[i - 1]->GetReadPipeFD(), nullptr,pgid);
        }
    }
    for (auto& pipe : pipeVector)
    {
        pipe->GetReadPipeFD()->Close();
        pipe->GetWritePipeFD()->Close();
    }
    for (size_t i = 0; i < pipeSize; i++)
    {
        statusVector[i] = std::make_unique<WaitStatus>(processVector[i]->GetPid());
    }
    if (!pipefail)
    {
        const auto& status = statusVector[pipeSize - 1];
        if (status->IsValid())
        {
            if (status->Signaled())
            {
                return status->GetSignal();
            }
            if (status->Exited())
            {
                if (status->ExitCode() != 0)
                {
                        return status->ExitCode();
                }
            }
        }
        else
        {
            return INVALID_STATUS;
        }
    }
    else
    {
        for (auto& status : statusVector)
        {
            if (status->IsValid())
            {
                if (status->Signaled())
                {
                    return status->GetSignal();
                }
                if (status->Exited())
                {
                    if (status->ExitCode() != 0)
                    {
                            return status->ExitCode();
                    }
                }
            }
            else
            {
                return INVALID_STATUS;
            }
        }
    }
    return 0;
}
} // namespace exec