#include <gtest/gtest.h>
#include <string>

#include "parser/Token.hpp"

using namespace parser;

// ─── Construction ─────────────────────────────────────────────────────────────

TEST(TokenTest, DefaultFieldValues)
{
    Token t;
    t.type = TokenType::Word;

    EXPECT_EQ(t.fd,   -1);
    EXPECT_EQ(t.line,  0);
    EXPECT_EQ(t.col,   0);
    EXPECT_TRUE(t.value.empty());
}

TEST(TokenTest, FieldsSetCorrectly)
{
    Token t;
    t.type  = TokenType::Word;
    t.value = "grep";
    t.fd    = -1;
    t.line  = 3;
    t.col   = 7;

    EXPECT_EQ(t.type,  TokenType::Word);
    EXPECT_EQ(t.value, "grep");
    EXPECT_EQ(t.fd,    -1);
    EXPECT_EQ(t.line,   3);
    EXPECT_EQ(t.col,    7);
}

// ─── Copyability ─────────────────────────────────────────────────────────────
// Tokens are value types — the parser copies them freely.

TEST(TokenTest, TokenIsCopyConstructible)
{
    Token a;
    a.type  = TokenType::Word;
    a.value = "cat";
    a.line  = 1;
    a.col   = 1;

    Token b = a;
    EXPECT_EQ(b.type,  TokenType::Word);
    EXPECT_EQ(b.value, "cat");
    EXPECT_EQ(b.line,   1);
    EXPECT_EQ(b.col,    1);
}

TEST(TokenTest, TokenIsCopyAssignable)
{
    Token a;
    a.type  = TokenType::Pipe;
    a.line  = 2;

    Token b;
    b = a;
    EXPECT_EQ(b.type, TokenType::Pipe);
    EXPECT_EQ(b.line, 2);
}

TEST(TokenTest, CopyIsIndependent)
{
    Token a;
    a.type  = TokenType::Word;
    a.value = "hello";

    Token b = a;
    b.value = "world";

    EXPECT_EQ(a.value, "hello");
    EXPECT_EQ(b.value, "world");
}

// ─── Word tokens ──────────────────────────────────────────────────────────────

TEST(TokenTest, WordTokenCarriesValue)
{
    Token t;
    t.type  = TokenType::Word;
    t.value = "file.txt";

    EXPECT_EQ(t.type,  TokenType::Word);
    EXPECT_EQ(t.value, "file.txt");
}

TEST(TokenTest, SingleQuotedTokenCarriesLiteralValue)
{
    Token t;
    t.type  = TokenType::SingleQuoted;
    t.value = "hello world";   // spaces preserved, no expansion

    EXPECT_EQ(t.type,  TokenType::SingleQuoted);
    EXPECT_EQ(t.value, "hello world");
}

TEST(TokenTest, DoubleQuotedTokenCarriesRawValue)
{
    Token t;
    t.type  = TokenType::DoubleQuoted;
    t.value = "hello $USER";   // raw — executor expands later

    EXPECT_EQ(t.type,  TokenType::DoubleQuoted);
    EXPECT_EQ(t.value, "hello $USER");
}

// ─── Operator tokens (no value needed) ───────────────────────────────────────

TEST(TokenTest, PipeTokenNeedsNoValue)
{
    Token t;
    t.type = TokenType::Pipe;

    EXPECT_EQ(t.type, TokenType::Pipe);
    EXPECT_TRUE(t.value.empty());
    EXPECT_EQ(t.fd, -1);
}

TEST(TokenTest, PipeBothIsDistinctFromPipe)
{
    Token pipe, pipeBoth;
    pipe.type     = TokenType::Pipe;
    pipeBoth.type = TokenType::PipeBoth;

    EXPECT_NE(pipe.type, pipeBoth.type);
}

TEST(TokenTest, AndIsDistinctFromBackground)
{
    Token logical, bg;
    logical.type = TokenType::And;
    bg.type      = TokenType::Background;

    EXPECT_NE(logical.type, bg.type);
}

TEST(TokenTest, OrIsDistinctFromPipe)
{
    Token or_, pipe;
    or_.type  = TokenType::Or;
    pipe.type = TokenType::Pipe;

    EXPECT_NE(or_.type, pipe.type);
}

// ─── Redirect tokens and fd field ────────────────────────────────────────────

TEST(TokenTest, RedirOutDefaultFdIsMinusOne)
{
    // Default redirect — lexer leaves fd=-1 meaning "use default for type" (fd 1)
    Token t;
    t.type = TokenType::RedirOut;

    EXPECT_EQ(t.fd, -1);
}

TEST(TokenTest, ExplicitFdStoredCorrectly)
{
    // 2> — stderr redirect
    Token t;
    t.type = TokenType::RedirOut;
    t.fd   = 2;

    EXPECT_EQ(t.type, TokenType::RedirOut);
    EXPECT_EQ(t.fd,   2);
}

TEST(TokenTest, DupOutCarriesTargetInValue)
{
    // 2>&1 — dup stderr onto stdout
    Token t;
    t.type  = TokenType::DupOut;
    t.fd    = 2;        // source fd
    t.value = "1";      // target fd as string

    EXPECT_EQ(t.type,  TokenType::DupOut);
    EXPECT_EQ(t.fd,     2);
    EXPECT_EQ(t.value, "1");
}

TEST(TokenTest, DupInCarriesTargetInValue)
{
    // 0<&3 — dup fd 3 onto stdin
    Token t;
    t.type  = TokenType::DupIn;
    t.fd    = 0;
    t.value = "3";

    EXPECT_EQ(t.fd,    0);
    EXPECT_EQ(t.value, "3");
}

TEST(TokenTest, HereDocCarriesDelimiter)
{
    // <<EOF — delimiter stored in value; body added later by lexer
    Token t;
    t.type  = TokenType::HereDoc;
    t.value = "EOF";

    EXPECT_EQ(t.type,  TokenType::HereDoc);
    EXPECT_EQ(t.value, "EOF");
}

TEST(TokenTest, HereStringCarriesBody)
{
    Token t;
    t.type  = TokenType::HereString;
    t.value = "hello world";

    EXPECT_EQ(t.type,  TokenType::HereString);
    EXPECT_EQ(t.value, "hello world");
}

TEST(TokenTest, RedirBothAndRedirBothAppendAreDistinct)
{
    Token both, bothAppend;
    both.type       = TokenType::RedirBoth;
    bothAppend.type = TokenType::RedirBothAppend;

    EXPECT_NE(both.type, bothAppend.type);
}

TEST(TokenTest, HereDocDashDistinctFromHereDoc)
{
    Token hd, hdd;
    hd.type  = TokenType::HereDoc;
    hdd.type = TokenType::HereDocDash;

    EXPECT_NE(hd.type, hdd.type);
}

// ─── Keyword tokens ───────────────────────────────────────────────────────────

TEST(TokenTest, KeywordsAreDistinctFromWord)
{
    Token word, kw;
    word.type  = TokenType::Word;
    word.value = "if";
    kw.type    = TokenType::If;

    // The parser decides which is correct based on position —
    // but the types themselves are always distinct.
    EXPECT_NE(word.type, kw.type);
}

TEST(TokenTest, BangIsItsOwnType)
{
    Token t;
    t.type = TokenType::Bang;

    EXPECT_EQ(t.type, TokenType::Bang);
    EXPECT_TRUE(t.value.empty());
}

// ─── Grouping tokens ──────────────────────────────────────────────────────────

TEST(TokenTest, DoubleBracketDistinctFromSingle)
{
    Token single, dbl;
    single.type = TokenType::LBrace;
    dbl.type    = TokenType::DLBracket;

    EXPECT_NE(single.type, dbl.type);
}

// ─── Structural ───────────────────────────────────────────────────────────────

TEST(TokenTest, EofToken)
{
    Token t;
    t.type = TokenType::Eof;

    EXPECT_EQ(t.type,  TokenType::Eof);
    EXPECT_TRUE(t.value.empty());
    EXPECT_EQ(t.fd, -1);
}

// ─── tokenTypeName ────────────────────────────────────────────────────────────

TEST(TokenTest, TypeNameWord)           { EXPECT_EQ(tokenTypeName(TokenType::Word),            "Word"); }
TEST(TokenTest, TypeNameSingleQuoted)   { EXPECT_EQ(tokenTypeName(TokenType::SingleQuoted),    "SingleQuoted"); }
TEST(TokenTest, TypeNameDoubleQuoted)   { EXPECT_EQ(tokenTypeName(TokenType::DoubleQuoted),    "DoubleQuoted"); }
TEST(TokenTest, TypeNamePipe)           { EXPECT_EQ(tokenTypeName(TokenType::Pipe),            "Pipe"); }
TEST(TokenTest, TypeNamePipeBoth)       { EXPECT_EQ(tokenTypeName(TokenType::PipeBoth),        "PipeBoth"); }
TEST(TokenTest, TypeNameAnd)            { EXPECT_EQ(tokenTypeName(TokenType::And),             "And"); }
TEST(TokenTest, TypeNameOr)             { EXPECT_EQ(tokenTypeName(TokenType::Or),              "Or"); }
TEST(TokenTest, TypeNameSemi)           { EXPECT_EQ(tokenTypeName(TokenType::Semi),            "Semi"); }
TEST(TokenTest, TypeNameDoubleSemi)     { EXPECT_EQ(tokenTypeName(TokenType::DoubleSemi),      "DoubleSemi"); }
TEST(TokenTest, TypeNameSemiAmp)        { EXPECT_EQ(tokenTypeName(TokenType::SemiAmp),         "SemiAmp"); }
TEST(TokenTest, TypeNameDoubleSemiAmp)  { EXPECT_EQ(tokenTypeName(TokenType::DoubleSemiAmp),   "DoubleSemiAmp"); }
TEST(TokenTest, TypeNameBackground)     { EXPECT_EQ(tokenTypeName(TokenType::Background),      "Background"); }
TEST(TokenTest, TypeNameNewline)        { EXPECT_EQ(tokenTypeName(TokenType::Newline),         "Newline"); }
TEST(TokenTest, TypeNameRedirIn)        { EXPECT_EQ(tokenTypeName(TokenType::RedirIn),         "RedirIn"); }
TEST(TokenTest, TypeNameRedirReadWrite) { EXPECT_EQ(tokenTypeName(TokenType::RedirReadWrite),  "RedirReadWrite"); }
TEST(TokenTest, TypeNameHereDoc)        { EXPECT_EQ(tokenTypeName(TokenType::HereDoc),         "HereDoc"); }
TEST(TokenTest, TypeNameHereDocDash)    { EXPECT_EQ(tokenTypeName(TokenType::HereDocDash),     "HereDocDash"); }
TEST(TokenTest, TypeNameHereString)     { EXPECT_EQ(tokenTypeName(TokenType::HereString),      "HereString"); }
TEST(TokenTest, TypeNameDupIn)          { EXPECT_EQ(tokenTypeName(TokenType::DupIn),           "DupIn"); }
TEST(TokenTest, TypeNameRedirOut)       { EXPECT_EQ(tokenTypeName(TokenType::RedirOut),        "RedirOut"); }
TEST(TokenTest, TypeNameRedirAppend)    { EXPECT_EQ(tokenTypeName(TokenType::RedirAppend),     "RedirAppend"); }
TEST(TokenTest, TypeNameRedirClobber)   { EXPECT_EQ(tokenTypeName(TokenType::RedirClobber),    "RedirClobber"); }
TEST(TokenTest, TypeNameDupOut)         { EXPECT_EQ(tokenTypeName(TokenType::DupOut),          "DupOut"); }
TEST(TokenTest, TypeNameRedirBoth)      { EXPECT_EQ(tokenTypeName(TokenType::RedirBoth),       "RedirBoth"); }
TEST(TokenTest, TypeNameRedirBothApp)   { EXPECT_EQ(tokenTypeName(TokenType::RedirBothAppend), "RedirBothAppend"); }
TEST(TokenTest, TypeNameIf)             { EXPECT_EQ(tokenTypeName(TokenType::If),              "If"); }
TEST(TokenTest, TypeNameThen)           { EXPECT_EQ(tokenTypeName(TokenType::Then),            "Then"); }
TEST(TokenTest, TypeNameElif)           { EXPECT_EQ(tokenTypeName(TokenType::Elif),            "Elif"); }
TEST(TokenTest, TypeNameElse)           { EXPECT_EQ(tokenTypeName(TokenType::Else),            "Else"); }
TEST(TokenTest, TypeNameFi)             { EXPECT_EQ(tokenTypeName(TokenType::Fi),              "Fi"); }
TEST(TokenTest, TypeNameWhile)          { EXPECT_EQ(tokenTypeName(TokenType::While),           "While"); }
TEST(TokenTest, TypeNameUntil)          { EXPECT_EQ(tokenTypeName(TokenType::Until),           "Until"); }
TEST(TokenTest, TypeNameDo)             { EXPECT_EQ(tokenTypeName(TokenType::Do),              "Do"); }
TEST(TokenTest, TypeNameDone)           { EXPECT_EQ(tokenTypeName(TokenType::Done),            "Done"); }
TEST(TokenTest, TypeNameFor)            { EXPECT_EQ(tokenTypeName(TokenType::For),             "For"); }
TEST(TokenTest, TypeNameIn)             { EXPECT_EQ(tokenTypeName(TokenType::In),              "In"); }
TEST(TokenTest, TypeNameCase)           { EXPECT_EQ(tokenTypeName(TokenType::Case),            "Case"); }
TEST(TokenTest, TypeNameEsac)           { EXPECT_EQ(tokenTypeName(TokenType::Esac),            "Esac"); }
TEST(TokenTest, TypeNameSelect)         { EXPECT_EQ(tokenTypeName(TokenType::Select),          "Select"); }
TEST(TokenTest, TypeNameFunction)       { EXPECT_EQ(tokenTypeName(TokenType::Function),        "Function"); }
TEST(TokenTest, TypeNameTime)           { EXPECT_EQ(tokenTypeName(TokenType::Time),            "Time"); }
TEST(TokenTest, TypeNameBang)           { EXPECT_EQ(tokenTypeName(TokenType::Bang),            "Bang"); }
TEST(TokenTest, TypeNameLParen)         { EXPECT_EQ(tokenTypeName(TokenType::LParen),          "LParen"); }
TEST(TokenTest, TypeNameRParen)         { EXPECT_EQ(tokenTypeName(TokenType::RParen),          "RParen"); }
TEST(TokenTest, TypeNameLBrace)         { EXPECT_EQ(tokenTypeName(TokenType::LBrace),          "LBrace"); }
TEST(TokenTest, TypeNameRBrace)         { EXPECT_EQ(tokenTypeName(TokenType::RBrace),          "RBrace"); }
TEST(TokenTest, TypeNameDLParen)        { EXPECT_EQ(tokenTypeName(TokenType::DLParen),         "DLParen"); }
TEST(TokenTest, TypeNameDRParen)        { EXPECT_EQ(tokenTypeName(TokenType::DRParen),         "DRParen"); }
TEST(TokenTest, TypeNameDLBracket)      { EXPECT_EQ(tokenTypeName(TokenType::DLBracket),       "DLBracket"); }
TEST(TokenTest, TypeNameDRBracket)      { EXPECT_EQ(tokenTypeName(TokenType::DRBracket),       "DRBracket"); }
TEST(TokenTest, TypeNameEof)            { EXPECT_EQ(tokenTypeName(TokenType::Eof),             "Eof"); }
