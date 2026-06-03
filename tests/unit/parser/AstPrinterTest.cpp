// =========================================================
// AstPrinterTest — the AstPrinter visitor dumps a tree to stdout.
// We build trees with the parser and capture stdout to assert the
// printed structure contains the expected node labels.
// =========================================================

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "parser/Parser.hpp"
#include "parser/Tokenizer.hpp"
#include "parser/ast/AstPrinter.hpp"

namespace
{
std::string dump(const std::string& src)
{
    parser::Tokenizer tok(src);
    parser::Parser    p(tok.Tokenize());
    auto              tree = p.Parse();

    parser::ast::AstPrinter printer;
    testing::internal::CaptureStdout();
    tree->Accept(printer);
    return testing::internal::GetCapturedStdout();
}

bool contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}
}

TEST(AstPrinter, SimpleCommand)
{
    auto out = dump("ls -l");
    EXPECT_TRUE(contains(out, "SimpleCommand"));
    EXPECT_TRUE(contains(out, "ls"));
    EXPECT_TRUE(contains(out, "-l"));
}

TEST(AstPrinter, Pipeline)
{
    auto out = dump("ls | grep foo");
    EXPECT_TRUE(contains(out, "Pipeline"));
    EXPECT_TRUE(contains(out, "grep"));
}

TEST(AstPrinter, AndOr)
{
    auto out = dump("make && ls");
    EXPECT_TRUE(contains(out, "AndOr"));
}

TEST(AstPrinter, List)
{
    auto out = dump("a ; b ; c");
    EXPECT_TRUE(contains(out, "List"));
}

TEST(AstPrinter, If)
{
    auto out = dump("if true; then echo hi; fi");
    EXPECT_TRUE(contains(out, "If"));
    EXPECT_TRUE(contains(out, "echo"));
}

TEST(AstPrinter, While)
{
    auto out = dump("while true; do echo x; done");
    EXPECT_TRUE(contains(out, "While"));
}

TEST(AstPrinter, For)
{
    auto out = dump("for x in a b c; do echo $x; done");
    EXPECT_TRUE(contains(out, "For"));
}

TEST(AstPrinter, Case)
{
    auto out = dump("case $x in a) echo A;; esac");
    EXPECT_TRUE(contains(out, "Case"));
}

TEST(AstPrinter, Subshell)
{
    auto out = dump("( echo hi )");
    EXPECT_TRUE(contains(out, "Subshell"));
}

TEST(AstPrinter, Group)
{
    auto out = dump("{ echo hi ; }");
    EXPECT_TRUE(contains(out, "Group"));
}

TEST(AstPrinter, Function)
{
    auto out = dump("greet() { echo hi ; }");
    EXPECT_TRUE(contains(out, "Function"));
}
