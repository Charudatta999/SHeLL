// =========================================================
// ArithmeticEngineTest — unit tests for arithmetic::engine::Evaluate.
// Uses a map-backed fake ArithmeticVars so the engine is tested in
// isolation from ShellState.
// =========================================================

#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <string>

#include "arithmetic/ArithmeticEngine.hpp"
#include "arithmetic/ArithmeticException.hpp"
#include "arithmetic/ArithmeticVars.hpp"

namespace
{
class FakeVars final : public arithmetic::ArithmeticVars
{
public:
    std::optional<std::string> Get(const std::string& name) const override
    {
        auto it = m_vars_.find(name);
        if (it == m_vars_.end())
            return std::nullopt;
        return it->second;
    }
    void Set(const std::string& name, const std::string& value) override
    {
        m_vars_[name] = value;
    }
    std::map<std::string, std::string> m_vars_;
};

// Evaluate a pure expression with an empty variable store.
std::int64_t ev(const std::string& expr)
{
    FakeVars vars;
    return arithmetic::engine::Evaluate(expr, vars);
}
} // namespace

// ─── Basic arithmetic ────────────────────────────────────────────────────────
TEST(ArithmeticEngine, Addition)        { EXPECT_EQ(ev("2+3"), 5); }
TEST(ArithmeticEngine, Subtraction)     { EXPECT_EQ(ev("10-4"), 6); }
TEST(ArithmeticEngine, Multiplication)  { EXPECT_EQ(ev("6*7"), 42); }
TEST(ArithmeticEngine, Division)        { EXPECT_EQ(ev("20/4"), 5); }
TEST(ArithmeticEngine, IntegerDivision) { EXPECT_EQ(ev("10/3"), 3); }
TEST(ArithmeticEngine, Modulo)          { EXPECT_EQ(ev("10%3"), 1); }

// ─── Precedence & associativity ──────────────────────────────────────────────
TEST(ArithmeticEngine, MulBeforeAdd)    { EXPECT_EQ(ev("2+3*4"), 14); }
TEST(ArithmeticEngine, ParensOverride)  { EXPECT_EQ(ev("(2+3)*4"), 20); }
TEST(ArithmeticEngine, LeftAssocMinus)  { EXPECT_EQ(ev("10-2-3"), 5); }
TEST(ArithmeticEngine, NestedParens)    { EXPECT_EQ(ev("2*(3+(4-1))"), 12); }
TEST(ArithmeticEngine, Whitespace)      { EXPECT_EQ(ev("  2  +  3  "), 5); }

// ─── Unary minus ─────────────────────────────────────────────────────────────
TEST(ArithmeticEngine, UnaryNeg)        { EXPECT_EQ(ev("-5"), -5); }
TEST(ArithmeticEngine, NegAfterOp)      { EXPECT_EQ(ev("2*-3"), -6); }
TEST(ArithmeticEngine, NegInParens)     { EXPECT_EQ(ev("(-3)+1"), -2); }
TEST(ArithmeticEngine, BinaryVsUnary)   { EXPECT_EQ(ev("5- -3"), 8); }

// ─── Comparisons & logical ───────────────────────────────────────────────────
TEST(ArithmeticEngine, LessTrue)        { EXPECT_EQ(ev("2<3"), 1); }
TEST(ArithmeticEngine, LessFalse)       { EXPECT_EQ(ev("3<2"), 0); }
TEST(ArithmeticEngine, Equal)           { EXPECT_EQ(ev("2==2"), 1); }
TEST(ArithmeticEngine, NotEqual)        { EXPECT_EQ(ev("2!=3"), 1); }
TEST(ArithmeticEngine, GreaterEqual)    { EXPECT_EQ(ev("3>=3"), 1); }
TEST(ArithmeticEngine, LogicalAnd)      { EXPECT_EQ(ev("1&&0"), 0); }
TEST(ArithmeticEngine, LogicalOr)       { EXPECT_EQ(ev("1||0"), 1); }

// ─── Variables ───────────────────────────────────────────────────────────────
TEST(ArithmeticEngine, UnsetVarIsZero)  { EXPECT_EQ(ev("x+1"), 1); }

TEST(ArithmeticEngine, ReadsVar)
{
    FakeVars vars;
    vars.m_vars_["i"] = "10";
    EXPECT_EQ(arithmetic::engine::Evaluate("i+5", vars), 15);
}

// ─── Assignment ──────────────────────────────────────────────────────────────
TEST(ArithmeticEngine, AssignSetsAndReturns)
{
    FakeVars vars;
    EXPECT_EQ(arithmetic::engine::Evaluate("x = 5", vars), 5);
    EXPECT_EQ(vars.m_vars_["x"], "5");
}

TEST(ArithmeticEngine, AssignExpression)
{
    FakeVars vars;
    vars.m_vars_["i"] = "3";
    EXPECT_EQ(arithmetic::engine::Evaluate("x = i*2+1", vars), 7);
    EXPECT_EQ(vars.m_vars_["x"], "7");
}

TEST(ArithmeticEngine, CompoundPlusEq)
{
    FakeVars vars;
    vars.m_vars_["s"] = "10";
    EXPECT_EQ(arithmetic::engine::Evaluate("s += 5", vars), 15);
    EXPECT_EQ(vars.m_vars_["s"], "15");
}

TEST(ArithmeticEngine, CompoundStarEq)
{
    FakeVars vars;
    vars.m_vars_["s"] = "4";
    EXPECT_EQ(arithmetic::engine::Evaluate("s *= 3", vars), 12);
    EXPECT_EQ(vars.m_vars_["s"], "12");
}

// ─── Increment / decrement ───────────────────────────────────────────────────
TEST(ArithmeticEngine, PostIncrementReturnsOld)
{
    FakeVars vars;
    vars.m_vars_["i"] = "5";
    EXPECT_EQ(arithmetic::engine::Evaluate("i++", vars), 5);   // returns old
    EXPECT_EQ(vars.m_vars_["i"], "6");                          // stored new
}

TEST(ArithmeticEngine, PreIncrementReturnsNew)
{
    FakeVars vars;
    vars.m_vars_["i"] = "5";
    EXPECT_EQ(arithmetic::engine::Evaluate("++i", vars), 6);
    EXPECT_EQ(vars.m_vars_["i"], "6");
}

TEST(ArithmeticEngine, PostDecrement)
{
    FakeVars vars;
    vars.m_vars_["i"] = "5";
    EXPECT_EQ(arithmetic::engine::Evaluate("i--", vars), 5);
    EXPECT_EQ(vars.m_vars_["i"], "4");
}

// ─── Errors ──────────────────────────────────────────────────────────────────
TEST(ArithmeticEngine, DivByZeroThrows)
{
    EXPECT_THROW(ev("1/0"), arithmetic::ArithmeticException);
}

TEST(ArithmeticEngine, ModByZeroThrows)
{
    EXPECT_THROW(ev("1%0"), arithmetic::ArithmeticException);
}

TEST(ArithmeticEngine, BadCharThrows)
{
    EXPECT_THROW(ev("2 @ 3"), arithmetic::ArithmeticException);
}

// ─── Bitwise & shift ─────────────────────────────────────────────────────────
TEST(ArithmeticEngine, BitAnd)    { EXPECT_EQ(ev("6&3"), 2); }
TEST(ArithmeticEngine, BitOr)     { EXPECT_EQ(ev("6|1"), 7); }
TEST(ArithmeticEngine, BitXor)    { EXPECT_EQ(ev("5^1"), 4); }
TEST(ArithmeticEngine, BitNot)    { EXPECT_EQ(ev("~0"), -1); }
TEST(ArithmeticEngine, ShiftLeft) { EXPECT_EQ(ev("1<<4"), 16); }
TEST(ArithmeticEngine, ShiftRight){ EXPECT_EQ(ev("256>>2"), 64); }
TEST(ArithmeticEngine, BitPrec)   { EXPECT_EQ(ev("1|2&2"), 3); }   // & tighter than |

// ─── Power (right-assoc, binds tighter than unary minus) ─────────────────────
TEST(ArithmeticEngine, Power)        { EXPECT_EQ(ev("2**10"), 1024); }
TEST(ArithmeticEngine, PowerZero)    { EXPECT_EQ(ev("5**0"), 1); }
TEST(ArithmeticEngine, PowerRightAssoc){ EXPECT_EQ(ev("2**3**2"), 512); } // 2**(3**2)
TEST(ArithmeticEngine, NegPower)     { EXPECT_EQ(ev("-2**2"), -4); }      // -(2**2)
TEST(ArithmeticEngine, PowerNegExpThrows){ EXPECT_THROW(ev("2**-1"), arithmetic::ArithmeticException); }

// ─── Logical not ─────────────────────────────────────────────────────────────
TEST(ArithmeticEngine, NotZero)   { EXPECT_EQ(ev("!0"), 1); }
TEST(ArithmeticEngine, NotNonzero){ EXPECT_EQ(ev("!5"), 0); }
TEST(ArithmeticEngine, DoubleNot) { EXPECT_EQ(ev("!!5"), 1); }

// ─── Ternary ─────────────────────────────────────────────────────────────────
TEST(ArithmeticEngine, TernaryTrue)  { EXPECT_EQ(ev("1?2:3"), 2); }
TEST(ArithmeticEngine, TernaryFalse) { EXPECT_EQ(ev("0?2:3"), 3); }
TEST(ArithmeticEngine, TernaryNested) { EXPECT_EQ(ev("0?1:2?3:4"), 3); }

// ─── Short-circuit (dead branch must NOT evaluate -> no div-by-zero) ──────────
TEST(ArithmeticEngine, OrShortCircuits)   { EXPECT_EQ(ev("1||(1/0)"), 1); }
TEST(ArithmeticEngine, AndShortCircuits)  { EXPECT_EQ(ev("0&&(1/0)"), 0); }
TEST(ArithmeticEngine, TernaryShortCircuit){ EXPECT_EQ(ev("0?1/0:5"), 5); }

TEST(ArithmeticEngine, AndSideEffectSkipped)
{
    FakeVars vars;
    vars.m_vars_["i"] = "5";
    arithmetic::engine::Evaluate("0 && (i = 99)", vars);   // RHS skipped
    EXPECT_EQ(vars.m_vars_["i"], "5");                      // unchanged
}

// ─── Compound bitwise/shift assigns ──────────────────────────────────────────
TEST(ArithmeticEngine, AndEq)
{
    FakeVars vars; vars.m_vars_["x"] = "6";
    EXPECT_EQ(arithmetic::engine::Evaluate("x &= 3", vars), 2);
    EXPECT_EQ(vars.m_vars_["x"], "2");
}
TEST(ArithmeticEngine, ShlEq)
{
    FakeVars vars; vars.m_vars_["x"] = "1";
    EXPECT_EQ(arithmetic::engine::Evaluate("x <<= 4", vars), 16);
    EXPECT_EQ(vars.m_vars_["x"], "16");
}

// ─── Embedded assignment (recursive descent handles it now) ──────────────────
TEST(ArithmeticEngine, EmbeddedAssign)
{
    FakeVars vars;
    EXPECT_EQ(arithmetic::engine::Evaluate("(x = 2) + 1", vars), 3);
    EXPECT_EQ(vars.m_vars_["x"], "2");
}

// ─── Robustness: malformed input throws cleanly, never crashes (R1/R6) ───────
TEST(ArithmeticEngine, EmptyThrows)        { EXPECT_THROW(ev(""), arithmetic::ArithmeticException); }
TEST(ArithmeticEngine, TrailingOpThrows)   { EXPECT_THROW(ev("2+"), arithmetic::ArithmeticException); }
TEST(ArithmeticEngine, LeadingBinOpThrows) { EXPECT_THROW(ev("*5"), arithmetic::ArithmeticException); }
TEST(ArithmeticEngine, BareOpThrows)       { EXPECT_THROW(ev("+"), arithmetic::ArithmeticException); }
TEST(ArithmeticEngine, OpenParenThrows)    { EXPECT_THROW(ev("(2+3"), arithmetic::ArithmeticException); }
TEST(ArithmeticEngine, ExtraParenThrows)   { EXPECT_THROW(ev("2+3)"), arithmetic::ArithmeticException); }
TEST(ArithmeticEngine, TwoOperandsThrows)  { EXPECT_THROW(ev("2 3"), arithmetic::ArithmeticException); }
TEST(ArithmeticEngine, DanglingTernaryThrows){ EXPECT_THROW(ev("1?2"), arithmetic::ArithmeticException); }

TEST(ArithmeticEngine, NonNumericVarIsZero)
{
    FakeVars vars; vars.m_vars_["x"] = "abc";   // stoll would throw -> wrapped to 0
    EXPECT_EQ(arithmetic::engine::Evaluate("x + 1", vars), 1);
}
TEST(ArithmeticEngine, EmptyVarIsZero)
{
    FakeVars vars; vars.m_vars_["x"] = "";
    EXPECT_EQ(arithmetic::engine::Evaluate("x + 5", vars), 5);
}
TEST(ArithmeticEngine, OversizedVarIsZero)
{
    FakeVars vars; vars.m_vars_["x"] = "99999999999999999999999999";
    EXPECT_NO_THROW(arithmetic::engine::Evaluate("x + 1", vars));   // out_of_range -> 0
}
