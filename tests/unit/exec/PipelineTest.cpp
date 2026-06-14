// =========================================================
// PipelineTest — Pipeline::Run end to end with real commands.
// Uses WaitMode::UntilExit (blocking, no terminal handoff) so
// the test runner isn't dragged into job-control.
// =========================================================

#include <gtest/gtest.h>

#include <vector>

#include "exec/ExecHelpers.hpp"
#include "exec/Pipeline.hpp"
#include "exec/WaitStatus.hpp"

namespace
{
exec::PipelineResult run(const std::vector<exec::CommandSpec>& stages)
{
    exec::Pipeline pipeline;
    return pipeline.Run(stages, /*pipefail=*/false,
                        exec::WaitMode::UntilExit);
}
} // namespace

TEST(Pipeline, SingleTrueExitsZero)
{
    auto result = run({exec::CommandSpec({"true"})});
    EXPECT_EQ(result.status, 0);
    EXPECT_EQ(result.state, exec::State::Done);
}

TEST(Pipeline, SingleFalseExitsNonZero)
{
    auto result = run({exec::CommandSpec({"false"})});
    EXPECT_EQ(result.status, 1);
    EXPECT_EQ(result.state, exec::State::Done);
}

TEST(Pipeline, ExitCodePropagates)
{
    auto result = run({exec::CommandSpec({"sh", "-c", "exit 42"})});
    EXPECT_EQ(result.status, 42);
}

TEST(Pipeline, LastStageStatusWins)
{
    // false | true  -> overall 0 (without pipefail, last stage decides)
    auto result = run({exec::CommandSpec({"false"}),
                       exec::CommandSpec({"true"})});
    EXPECT_EQ(result.status, 0);
    EXPECT_EQ(result.state, exec::State::Done);
}

TEST(Pipeline, MultiStageRunsThrough)
{
    // echo hi | cat  -> 0
    auto result = run({exec::CommandSpec({"echo", "hi"}),
                       exec::CommandSpec({"cat"})});
    EXPECT_EQ(result.status, 0);
}

TEST(Pipeline, EmptyPipelineIsZero)
{
    auto result = run({});
    EXPECT_EQ(result.status, 0);
}
