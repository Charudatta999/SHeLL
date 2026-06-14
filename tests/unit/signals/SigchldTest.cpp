// =========================================================
// SigchldTest — the async-signal flag: set by the handler,
// read-and-cleared by Consume().
// =========================================================

#include <gtest/gtest.h>

#include <csignal>

#include "signals/Sigchld.hpp"

TEST(Sigchld, ConsumeIsFalseWhenNothingHappened)
{
    signals::Sigchld::Install();
    (void)signals::Sigchld::Consume(); // clear anything pending from earlier forks
    EXPECT_FALSE(signals::Sigchld::Consume());
}

TEST(Sigchld, SignalSetsFlagOnce)
{
    signals::Sigchld::Install();
    (void)signals::Sigchld::Consume(); // clean slate

    ::raise(SIGCHLD);            // deliver to self -> handler sets the flag

    EXPECT_TRUE(signals::Sigchld::Consume());  // observed once
    EXPECT_FALSE(signals::Sigchld::Consume()); // and cleared
}
