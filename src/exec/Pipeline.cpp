#include "exec/Pipeline.hpp"

#include "exec/ExecException.hpp"
#include "exec/ExecHelpers.hpp"
#include "exec/Process.hpp"
#include "io/FdOps.hpp"
#include "io/IOException.hpp"
#include "io/Pipe.hpp"
#include "utils/ErrorCodes.hpp"

#include <cerrno>
#include <csignal>
#include <cstddef>
#include <memory>
#include <sched.h>
#include <sys/syslog.h>
#include <syslog.h>
#include <unistd.h>


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
               "SHELL [exec] [Pipeline]: Failed to create pipe: %s "
               "(errno=%d)",
               ex.what(),
               ex.GetErrorCode());
        throw ExecException("failed To create pipe",
                            FAILED_TO_CREATE);
    }
}

PipelineResult Pipeline::Run(const std::vector<CommandSpec>& pipeline,
                  bool pipefail,
                  WaitMode mode)
{
    std::vector<std::unique_ptr<io::Pipe>> pipeVector;
    const size_t pipeSize = pipeline.size();
    if (pipeSize == 0)
        return {.status = 0, .pgid = -1, .state = State::Done};
    try
    {
        for (size_t i = 0; i < pipeSize - 1; i++)
        {
            pipeVector.push_back(CreatePipe());
        }
    }
    catch (const ExecException& ex)
    {
        auto error =
            std::string(
                "SHELL [exec] [Pipeline] : Failed to run command: ") +
            std::string(ex.what()) +
            " with errno: " + std::to_string(errno);
        throw ExecException(error, ex.GetErrorCode());
    }
    std::vector<std::unique_ptr<Process>> processVector(pipeSize);
    std::vector<std::unique_ptr<WaitStatus>> statusVector(pipeSize);
    if (pipeSize == 1)
    {
        Process process;
        process.Start(pipeline[0], nullptr, nullptr, 0);
        if (mode == WaitMode::Foreground)
        {
            // Lend the terminal
            //(keyboard to process as it is a foreground
            // process )
            tcsetpgrp(STDIN_FILENO, process.GetPid());
        }
        WaitStatus status(process.GetPid(), mode);
        if (mode == WaitMode::Foreground)
        {
            // Take back the terminal
            // as process execution is completed, attach it back to our
            // shell
            tcsetpgrp(STDIN_FILENO, getpgrp());
        }
        if (status.IsStopped())
            return {.status = SIGNAL_EXIT_BASE + SIGTSTP,
                    .pgid = process.GetPid(),
                    .state = State::Stopped};
        if (status.Signaled())
            return {.status = status.GetSignal(), .pgid = process.GetPid(), .state = State::Done};
        if (status.Exited())
            return {.status = status.ExitCode(), .pgid = process.GetPid(), .state = State::Done};
        return {.status = INVALID_STATUS, .pgid = process.GetPid(), .state = State::Done};
    }
    for (size_t i = 0; i < pipeSize; i++)
    {
        processVector[i] = std::make_unique<Process>();
        if (i == 0)
        {
            processVector[i]->Start(pipeline[i],
                                    nullptr,
                                    pipeVector[i]->GetWritePipeFD(),
                                    0);
        }
        else if (i > 0 && i < pipeSize - 1)
        {
            auto pgid = processVector[0]->GetPid();
            processVector[i]->Start(
                pipeline[i],
                pipeVector[i - 1]->GetReadPipeFD(),
                pipeVector[i]->GetWritePipeFD(),
                pgid);
        }
        else if (i == pipeSize - 1)
        {
            auto pgid = processVector[0]->GetPid();
            processVector[i]->Start(
                pipeline[i],
                pipeVector[i - 1]->GetReadPipeFD(),
                nullptr,
                pgid);
        }
    }
    for (auto& pipe : pipeVector)
    {
        pipe->GetReadPipeFD()->Close();
        pipe->GetWritePipeFD()->Close();
    }
    if (mode == WaitMode::Foreground)
    {
        // Lend the terminal
        //(keyboard to process as it is a foreground
        // process )
        tcsetpgrp(STDIN_FILENO, processVector[0]->GetPid());
    }
    for (size_t i = 0; i < pipeSize; i++)
    {
        statusVector[i] =
            std::make_unique<WaitStatus>(processVector[i]->GetPid(), mode);
    }
    if (mode == WaitMode::Foreground)
    {
        // Take back the terminal
        // as process execution is completed attach it back to our
        // shell
        tcsetpgrp(STDIN_FILENO, getpgrp());
    }
    // Last stage stopped => the job stopped (stage 0 leads the group).
    if (statusVector[pipeSize - 1]->IsStopped())
        return {.status = SIGNAL_EXIT_BASE + SIGTSTP,
                .pgid = processVector[0]->GetPid(),
                .state = State::Stopped};
    if (!pipefail)
    {
        const auto& status = statusVector[pipeSize - 1];
        if (status->IsValid())
        {
            if (status->Signaled())
            {
                return {.status = status->GetSignal(), .pgid = processVector[0]->GetPid(), .state = State::Done};
            }
            if (status->Exited())
            {
                if (status->ExitCode() != 0)
                {
                     return {.status = status->ExitCode(), .pgid = processVector[0]->GetPid(), .state = State::Done};
                }
            }
        }
        else
        {
            return {.status = INVALID_STATUS, .pgid =processVector[0]->GetPid(), .state = State::Done};
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
                    return {.status = status->GetSignal(), .pgid = processVector[0]->GetPid(), .state = State::Done};
                }
                if (status->Exited())
                {
                    if (status->ExitCode() != 0)
                    {
                        return {.status = status->ExitCode(), .pgid = processVector[0]->GetPid(), .state = State::Done};
                    }
                }
            }
            else
            {
                 return {.status = INVALID_STATUS, .pgid =processVector[0]->GetPid(), .state = State::Done};
            }
        }
    }
     return {.status = 0, .pgid =-1, .state = State::Done};
}
} // namespace exec