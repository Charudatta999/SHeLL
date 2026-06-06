// =========================================================
// CommandSpecTest — the value struct the Executor hands to Pipeline/Process.
// Pure data; verifies the three constructors populate fields correctly.
// =========================================================

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "exec/ExecHelpers.hpp"
#include "parser/ast/Redirect.hpp"

using exec::CommandSpec;
using parser::ast::Redirect;

TEST(CommandSpec, ArgvOnlyConstructor)
{
    CommandSpec s({"ls", "-l"});
    ASSERT_EQ(s.argv.size(), 2u);
    EXPECT_EQ(s.argv[0], "ls");
    EXPECT_EQ(s.argv[1], "-l");
    EXPECT_TRUE(s.redirects.empty());
    EXPECT_TRUE(s.envOverrides.empty());
}

TEST(CommandSpec, ArgvAndRedirects)
{
    std::vector<Redirect> redirs{{Redirect::Kind::Out, 1, "out.txt"}};
    CommandSpec s({"echo", "hi"}, redirs);
    EXPECT_EQ(s.argv.size(), 2u);
    ASSERT_EQ(s.redirects.size(), 1u);
    EXPECT_EQ(s.redirects[0].kind, Redirect::Kind::Out);
    EXPECT_EQ(s.redirects[0].target, "out.txt");
    EXPECT_TRUE(s.envOverrides.empty());
}

TEST(CommandSpec, FullConstructor)
{
    std::vector<Redirect> redirs{{Redirect::Kind::Append, 2, "log"}};
    std::vector<std::pair<std::string, std::string>> env{{"FOO", "bar"}};
    CommandSpec s({"cmd"}, redirs, env);

    EXPECT_EQ(s.argv[0], "cmd");
    ASSERT_EQ(s.redirects.size(), 1u);
    EXPECT_EQ(s.redirects[0].fd, 2);
    ASSERT_EQ(s.envOverrides.size(), 1u);
    EXPECT_EQ(s.envOverrides[0].first, "FOO");
    EXPECT_EQ(s.envOverrides[0].second, "bar");
}
