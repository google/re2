// Copyright 2007 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Test prog.cc, compile.cc

#include <string>
#include <vector>
#include "util/test.h"
#include "re2/regexp.h"
#include "re2/prog.h"

DEFINE_string(show, "", "regular expression to compile and dump");

namespace re2 {

// Simple input/output tests checking that
// the regexp compiles to the expected code.
// These are just to sanity check the basic implementation.
// The real confidence tests happen by testing the NFA/DFA
// that run the compiled code.

struct Test {
  const char* regexp;
  const char* code;
};

static Test tests[] = {
  { "a",
    "3. byte [61-61] -> 4\n"
    "4. match! 0\n" },
  { "ab",
    "3. byte [61-61] -> 4\n"
    "4. byte [62-62] -> 5\n"
    "5. match! 0\n" },
  { "a|c",
    "3+ byte [61-61] -> 5\n"
    "4. byte [63-63] -> 5\n"
    "5. match! 0\n" },
  { "a|b",
    "3. byte [61-62] -> 4\n"
    "4. match! 0\n" },
  { "[ab]",
    "3. byte [61-62] -> 4\n"
    "4. match! 0\n" },
  { "a+",
    "3. byte [61-61] -> 4\n"
    "4+ nop -> 3\n"
    "5. match! 0\n" },
  { "a+?",
    "3. byte [61-61] -> 4\n"
    "4+ match! 0\n"
    "5. nop -> 3\n" },
  { "a*",
    "3+ byte [61-61] -> 3\n"
    "4. match! 0\n" },
  { "a*?",
    "3+ match! 0\n"
    "4. byte [61-61] -> 3\n" },
  { "a?",
    "3+ byte [61-61] -> 5\n"
    "4. nop -> 5\n"
    "5. match! 0\n" },
  { "a??",
    "3+ nop -> 5\n"
    "4. byte [61-61] -> 5\n"
    "5. match! 0\n" },
  { "a{4}",
    "3. byte [61-61] -> 4\n"
    "4. byte [61-61] -> 5\n"
    "5. byte [61-61] -> 6\n"
    "6. byte [61-61] -> 7\n"
    "7. match! 0\n" },
  { "(a)",
    "3. capture 2 -> 4\n"
    "4. byte [61-61] -> 5\n"
    "5. capture 3 -> 6\n"
    "6. match! 0\n" },
  { "(?:a)",
    "3. byte [61-61] -> 4\n"
    "4. match! 0\n" },
  { "",
    "3. match! 0\n" },
  { ".",
    "3+ byte [00-09] -> 5\n"
    "4. byte [0b-ff] -> 5\n"
    "5. match! 0\n" },
  { "[^ab]",
    "3+ byte [00-09] -> 6\n"
    "4+ byte [0b-60] -> 6\n"
    "5. byte [63-ff] -> 6\n"
    "6. match! 0\n" },
  { "[Aa]",
    "3. byte/i [61-61] -> 4\n"
    "4. match! 0\n" },
  { "\\C+",
    "3. byte [00-ff] -> 4\n"
    "4+ altmatch -> 5 | 6\n"
    "5+ nop -> 3\n"
    "6. match! 0\n" },
  { "\\C*",
    "3+ altmatch -> 4 | 5\n"
    "4+ byte [00-ff] -> 3\n"
    "5. match! 0\n" },
  { "\\C?",
    "3+ byte [00-ff] -> 5\n"
    "4. nop -> 5\n"
    "5. match! 0\n" },
  // Issue 20992936
  { "[[-`]",
    "3. byte [5b-60] -> 4\n"
    "4. match! 0\n" },
};

TEST(TestRegexpCompileToProg, Simple) {
  int failed = 0;
  for (int i = 0; i < arraysize(tests); i++) {
    const re2::Test& t = tests[i];
    Regexp* re = Regexp::Parse(t.regexp, Regexp::PerlX|Regexp::Latin1, NULL);
    if (re == NULL) {
      LOG(ERROR) << "Cannot parse: " << t.regexp;
      failed++;
      continue;
    }
    Prog* prog = re->CompileToProg(0);
    if (prog == NULL) {
      LOG(ERROR) << "Cannot compile: " << t.regexp;
      re->Decref();
      failed++;
      continue;
    }
    CHECK(re->CompileToProg(1) == NULL);
    string s = prog->Dump();
    if (s != t.code) {
      LOG(ERROR) << "Incorrect compiled code for: " << t.regexp;
      LOG(ERROR) << "Want:\n" << t.code;
      LOG(ERROR) << "Got:\n" << s;
      failed++;
    }
    delete prog;
    re->Decref();
  }
  EXPECT_EQ(failed, 0);
}

// The distinct byte ranges involved in the UTF-8 dot ([^\n]).
// Once, erroneously split between 0x3f and 0x40 because it is
// a 6-bit boundary.
static struct UTF8ByteRange {
  int lo;
  int hi;
} utf8ranges[] = {
  { 0x00, 0x09 },
  { 0x0A, 0x0A },
  { 0x10, 0x7F },
  { 0x80, 0x8F },
  { 0x90, 0x9F },
  { 0xA0, 0xBF },
  { 0xC0, 0xC1 },
  { 0xC2, 0xDF },
  { 0xE0, 0xE0 },
  { 0xE1, 0xEF },
  { 0xF0, 0xF0 },
  { 0xF1, 0xF3 },
  { 0xF4, 0xF4 },
  { 0xF5, 0xFF },
};

TEST(TestCompile, ByteRanges) {
  Regexp* re = Regexp::Parse(".", Regexp::PerlX, NULL);
  EXPECT_TRUE(re != NULL);
  Prog* prog = re->CompileToProg(0);
  EXPECT_TRUE(prog != NULL);
  EXPECT_EQ(prog->bytemap_range(), arraysize(utf8ranges));
  for (int i = 0; i < arraysize(utf8ranges); i++)
    for (int j = utf8ranges[i].lo; j <= utf8ranges[i].hi; j++)
      EXPECT_EQ(prog->bytemap()[j], i) << " byte " << j;
  delete prog;
  re->Decref();
}

TEST(TestCompile, InsufficientMemory) {
  Regexp* re = Regexp::Parse(
      "^(?P<name1>[^\\s]+)\\s+(?P<name2>[^\\s]+)\\s+(?P<name3>.+)$",
      Regexp::LikePerl, NULL);
  EXPECT_TRUE(re != NULL);
  Prog* prog = re->CompileToProg(920);
  // If the memory budget has been exhausted, compilation should fail
  // and return NULL instead of trying to do anything with NoMatch().
  EXPECT_TRUE(prog == NULL);
  re->Decref();
}

static void Dump(StringPiece pattern, Regexp::ParseFlags flags,
                 string* forward, string* reverse) {
  Regexp* re = Regexp::Parse(pattern, flags, NULL);
  EXPECT_TRUE(re != NULL);

  if (forward != NULL) {
    Prog* prog = re->CompileToProg(0);
    EXPECT_TRUE(prog != NULL);
    *forward = prog->Dump();
    delete prog;
  }

  if (reverse != NULL) {
    Prog* prog = re->CompileToReverseProg(0);
    EXPECT_TRUE(prog != NULL);
    *reverse = prog->Dump();
    delete prog;
  }

  re->Decref();
}

TEST(TestCompile, Bug26705922) {
  // Bug in the compiler caused inefficient bytecode to be generated for Unicode
  // groups: common suffixes were cached, but common prefixes were not factored.

  string forward, reverse;

  Dump("[\\x{10000}\\x{10010}]", Regexp::LikePerl, &forward, &reverse);
  EXPECT_EQ("3. byte [f0-f0] -> 4\n"
            "4. byte [90-90] -> 5\n"
            "5. byte [80-80] -> 6\n"
            "6+ byte [80-80] -> 8\n"
            "7. byte [90-90] -> 8\n"
            "8. match! 0\n",
            forward);
  EXPECT_EQ("3+ byte [80-80] -> 5\n"
            "4. byte [90-90] -> 5\n"
            "5. byte [80-80] -> 6\n"
            "6. byte [90-90] -> 7\n"
            "7. byte [f0-f0] -> 8\n"
            "8. match! 0\n",
            reverse);

  Dump("[\\x{8000}-\\x{10FFF}]", Regexp::LikePerl, &forward, &reverse);
  EXPECT_EQ("3+ byte [e8-ef] -> 5\n"
            "4. byte [f0-f0] -> 8\n"
            "5. byte [80-bf] -> 6\n"
            "6. byte [80-bf] -> 7\n"
            "7. match! 0\n"
            "8. byte [90-90] -> 5\n",
            forward);
  EXPECT_EQ("3. byte [80-bf] -> 4\n"
            "4. byte [80-bf] -> 5\n"
            "5+ byte [e8-ef] -> 7\n"
            "6. byte [90-90] -> 8\n"
            "7. match! 0\n"
            "8. byte [f0-f0] -> 7\n",
            reverse);

  Dump("[\\x{80}-\\x{10FFFF}]", Regexp::LikePerl, NULL, &reverse);
  EXPECT_EQ("3. byte [80-bf] -> 4\n"
            "4+ byte [c2-df] -> 7\n"
            "5+ byte [a0-bf] -> 8\n"
            "6. byte [80-bf] -> 9\n"
            "7. match! 0\n"
            "8. byte [e0-e0] -> 7\n"
            "9+ byte [e1-ef] -> 7\n"
            "10+ byte [90-bf] -> 13\n"
            "11+ byte [80-bf] -> 14\n"
            "12. byte [80-8f] -> 15\n"
            "13. byte [f0-f0] -> 7\n"
            "14. byte [f1-f3] -> 7\n"
            "15. byte [f4-f4] -> 7\n",
            reverse);
}

}  // namespace re2
