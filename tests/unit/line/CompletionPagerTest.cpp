// =========================================================
// CompletionPagerTest — unit tests for line::CompletionPager.
// Tests: Open, Filter, Next/Prev/Column navigation, Empty,
// Selected, and ReplaceStart. Skips Render (requires TTY).
// =========================================================

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "line/Completer.hpp"
#include "line/CompletionPager.hpp"

namespace
{
/// @brief Build a Result with candidates for testing.
line::Result MakeResult(const std::vector<std::string>& texts,
                        std::size_t replaceStart = 0)
{
    std::vector<line::Candidate> candidates;
    candidates.reserve(texts.size());
    for (const auto& text : texts)
    {
        candidates.push_back({.text = text, .description = "test"});
    }
    return {.candidates = candidates, .replaceStart = replaceStart};
}

} // namespace

// ─── Open ────────────────────────────────────────────────────────────────────

TEST(CompletionPager, OpenStoresResultAndResetsSelection)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "avocado", "apricot"}, 5);

    pager.Open(result);

    EXPECT_FALSE(pager.Empty());
    EXPECT_EQ(pager.Selected().text, "apple"); // first candidate
    EXPECT_EQ(pager.ReplaceStart(), 5);
}

TEST(CompletionPager, OpenWithEmptyResult)
{
    line::CompletionPager pager;
    auto result = MakeResult({}, 0);

    pager.Open(result);

    EXPECT_TRUE(pager.Empty());
}

TEST(CompletionPager, OpenWithOneCandidate)
{
    line::CompletionPager pager;
    auto result = MakeResult({"single"}, 10);

    pager.Open(result);

    EXPECT_FALSE(pager.Empty());
    EXPECT_EQ(pager.Selected().text, "single");
    EXPECT_EQ(pager.ReplaceStart(), 10);
}

// ─── Filter ──────────────────────────────────────────────────────────────────

TEST(CompletionPager, FilterNarrowsByCandidatePrefix)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "apricot", "avocado", "banana"}, 0);
    pager.Open(result);

    pager.Filter("ap");

    EXPECT_FALSE(pager.Empty());
    EXPECT_EQ(pager.Selected().text, "apple");
}

TEST(CompletionPager, FilterResetsSelectionToFirst)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "avocado", "apricot"}, 0);
    pager.Open(result);

    // Move to the second item
    pager.Next();
    EXPECT_EQ(pager.Selected().text, "avocado");

    // Filter should reset to first match
    pager.Filter("ap");
    EXPECT_EQ(pager.Selected().text, "apple");
}

TEST(CompletionPager, FilterEmptyStringShowsAll)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "banana", "cherry"}, 0);
    pager.Open(result);

    pager.Filter("");

    EXPECT_FALSE(pager.Empty());
    EXPECT_EQ(pager.Selected().text, "apple");
}

TEST(CompletionPager, FilterToNoMatches)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "avocado"}, 0);
    pager.Open(result);

    pager.Filter("xyz");

    EXPECT_TRUE(pager.Empty());
}

TEST(CompletionPager, FilterIsIncrementalPrefixMatch)
{
    line::CompletionPager pager;
    auto result = MakeResult({"alpha", "alternate", "aloof"}, 0);
    pager.Open(result);

    pager.Filter("al");
    EXPECT_FALSE(pager.Empty());
    EXPECT_EQ(pager.Selected().text, "alpha"); // all three match, first wins

    pager.Filter("alt");
    // Only "alternate" starts with "alt"
    EXPECT_EQ(pager.Selected().text, "alternate");
}

// ─── Navigation: Next and Prev ────────────────────────────────────────────────

TEST(CompletionPager, NextMovesForward)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "banana", "cherry"}, 0);
    pager.Open(result);

    EXPECT_EQ(pager.Selected().text, "apple");
    pager.Next();
    EXPECT_EQ(pager.Selected().text, "banana");
    pager.Next();
    EXPECT_EQ(pager.Selected().text, "cherry");
}

TEST(CompletionPager, NextWrapsAroundAtEnd)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "banana"}, 0);
    pager.Open(result);

    pager.Next(); // to "banana", the last item
    pager.Next(); // wraps around to the front
    EXPECT_EQ(pager.Selected().text, "apple");
}

TEST(CompletionPager, PrevMovesBackward)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "banana", "cherry"}, 0);
    pager.Open(result);

    pager.Next();
    pager.Next();
    EXPECT_EQ(pager.Selected().text, "cherry");
    pager.Prev();
    EXPECT_EQ(pager.Selected().text, "banana");
}

TEST(CompletionPager, PrevWrapsAroundAtStart)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "banana"}, 0);
    pager.Open(result);

    pager.Prev(); // at start, wrap to end
    EXPECT_EQ(pager.Selected().text, "banana");
}

TEST(CompletionPager, NextAndPrevOnSingleItem)
{
    line::CompletionPager pager;
    auto result = MakeResult({"alone"}, 0);
    pager.Open(result);

    pager.Next();
    EXPECT_EQ(pager.Selected().text, "alone");
    pager.Prev();
    EXPECT_EQ(pager.Selected().text, "alone");
}

TEST(CompletionPager, NextOnEmptyDoesNotCrash)
{
    line::CompletionPager pager;
    auto result = MakeResult({}, 0);
    pager.Open(result);

    pager.Next(); // should not crash
    EXPECT_TRUE(pager.Empty());
}

TEST(CompletionPager, PrevOnEmptyDoesNotCrash)
{
    line::CompletionPager pager;
    auto result = MakeResult({}, 0);
    pager.Open(result);

    pager.Prev(); // should not crash
    EXPECT_TRUE(pager.Empty());
}

// ─── Column Navigation ───────────────────────────────────────────────────────

TEST(CompletionPager, NextColumnStepsByRowStride)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "banana"}, 0);
    pager.Open(result);

    // Without a Render the grid has one row, so a column step is one item.
    pager.NextColumn();
    EXPECT_EQ(pager.Selected().text, "banana");
    pager.NextColumn(); // wraps past the end
    EXPECT_EQ(pager.Selected().text, "apple");
}

TEST(CompletionPager, PrevColumnStepsByRowStride)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "banana"}, 0);
    pager.Open(result);

    pager.PrevColumn(); // wraps backward from the start
    EXPECT_EQ(pager.Selected().text, "banana");
}

// ─── Selected ─────────────────────────────────────────────────────────────────

TEST(CompletionPager, SelectedAfterNavigation)
{
    line::CompletionPager pager;
    auto result =
        MakeResult({"apple", "banana", "cherry"}, 0);
    pager.Open(result);

    pager.Next();
    const auto& selected = pager.Selected();
    EXPECT_EQ(selected.text, "banana");
    EXPECT_EQ(selected.description, "test");
}

TEST(CompletionPager, SelectedAfterFilter)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "apricot", "banana"}, 0);
    pager.Open(result);

    pager.Filter("ap");
    pager.Next();
    EXPECT_EQ(pager.Selected().text, "apricot");
}

// ─── ReplaceStart ────────────────────────────────────────────────────────────

TEST(CompletionPager, ReplaceStartIsSetByOpen)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple"}, 42);
    pager.Open(result);

    EXPECT_EQ(pager.ReplaceStart(), 42);
}

TEST(CompletionPager, ReplaceStartDoesNotChangeAfterNavigation)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "banana"}, 10);
    pager.Open(result);

    pager.Next();
    pager.Filter("b");
    pager.Prev();

    EXPECT_EQ(pager.ReplaceStart(), 10);
}

// ─── Empty ───────────────────────────────────────────────────────────────────

TEST(CompletionPager, EmptyInitiallyFalseWithCandidates)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple"}, 0);
    pager.Open(result);

    EXPECT_FALSE(pager.Empty());
}

TEST(CompletionPager, EmptyInitiallyTrueWithoutCandidates)
{
    line::CompletionPager pager;
    auto result = MakeResult({}, 0);
    pager.Open(result);

    EXPECT_TRUE(pager.Empty());
}

TEST(CompletionPager, EmptyBecomesTrueAfterFilterRemovesAll)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "banana"}, 0);
    pager.Open(result);

    EXPECT_FALSE(pager.Empty());
    pager.Filter("cherry");
    EXPECT_TRUE(pager.Empty());
}

TEST(CompletionPager, EmptyBecomesFalseAfterFilterMatches)
{
    line::CompletionPager pager;
    auto result = MakeResult({"apple", "banana"}, 0);
    pager.Open(result);

    pager.Filter("cherry");
    EXPECT_TRUE(pager.Empty());
    pager.Filter("ap");
    EXPECT_FALSE(pager.Empty());
}
