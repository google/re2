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
    "1. byte [61-61] -> 2\n"
    "2. match! 0\n" },
  { "ab",
    "1. byte [61-61] -> 2\n"
    "2. byte [62-62] -> 3\n"
    "3. match! 0\n" },
  { "a|c",
    "3. alt -> 1 | 2\n"
    "1. byte [61-61] -> 4\n"
    "2. byte [63-63] -> 4\n"
    "4. match! 0\n" },
  { "a|b",
    "1. byte [61-62] -> 2\n"
    "2. match! 0\n" },
  { "[ab]",
    "1. byte [61-62] -> 2\n"
    "2. match! 0\n" },
  { "a+",
    "1. byte [61-61] -> 2\n"
    "2. alt -> 1 | 3\n"
    "3. match! 0\n" },
  { "a+?",
    "1. byte [61-61] -> 2\n"
    "2. alt -> 3 | 1\n"
    "3. match! 0\n" },
  { "a*",
    "2. alt -> 1 | 3\n"
    "1. byte [61-61] -> 2\n"
    "3. match! 0\n" },
  { "a*?",
    "2. alt -> 3 | 1\n"
    "3. match! 0\n"
    "1. byte [61-61] -> 2\n" },
  { "a?",
    "2. alt -> 1 | 3\n"
    "1. byte [61-61] -> 3\n"
    "3. match! 0\n" },
  { "a??",
    "2. alt -> 3 | 1\n"
    "3. match! 0\n"
    "1. byte [61-61] -> 3\n" },
  { "a{4}",
    "1. byte [61-61] -> 2\n"
    "2. byte [61-61] -> 3\n"
    "3. byte [61-61] -> 4\n"
    "4. byte [61-61] -> 5\n"
    "5. match! 0\n" },
  { "(a)",
    "2. capture 2 -> 1\n"
    "1. byte [61-61] -> 3\n"
    "3. capture 3 -> 4\n"
    "4. match! 0\n" },
  { "(?:a)",
    "1. byte [61-61] -> 2\n"
    "2. match! 0\n" },
  { "",
    "2. match! 0\n" },
  { ".",
    "3. alt -> 1 | 2\n"
    "1. byte [00-09] -> 4\n"
    "2. byte [0b-ff] -> 4\n"
    "4. match! 0\n" },
  { "[^ab]",
    "5. alt -> 3 | 4\n"
    "3. alt -> 1 | 2\n"
    "4. byte [63-ff] -> 6\n"
    "1. byte [00-09] -> 6\n"
    "2. byte [0b-60] -> 6\n"
    "6. match! 0\n" },
  { "[Aa]",
    "1. byte/i [61-61] -> 2\n"
    "2. match! 0\n" },
  // Issue 20992936
  { "[[-`]",
    "1. byte [5b-60] -> 2\n"
    "2. match! 0\n" },
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
  EXPECT_EQ("4. byte [f0-f0] -> 3\n"
            "3. byte [90-90] -> 2\n"
            "2. byte [80-80] -> 6\n"
            "6. alt -> 1 | 5\n"
            "1. byte [80-80] -> 7\n"
            "5. byte [90-90] -> 7\n"
            "7. match! 0\n",
            forward);
  EXPECT_EQ("6. alt -> 4 | 5\n"
            "4. byte [80-80] -> 3\n"
            "5. byte [90-90] -> 3\n"
            "3. byte [80-80] -> 2\n"
            "2. byte [90-90] -> 1\n"
            "1. byte [f0-f0] -> 7\n"
            "7. match! 0\n",
            reverse);

  Dump("[\\x{8000}-\\x{10FFF}]", Regexp::LikePerl, &forward, &reverse);
  EXPECT_EQ("6. alt -> 3 | 5\n"
            "3. byte [e8-ef] -> 2\n"
            "5. byte [f0-f0] -> 4\n"
            "2. byte [80-bf] -> 1\n"
            "4. byte [90-90] -> 2\n"
            "1. byte [80-bf] -> 7\n"
            "7. match! 0\n",
            forward);
  EXPECT_EQ("3. byte [80-bf] -> 2\n"
            "2. byte [80-bf] -> 6\n"
            "6. alt -> 1 | 5\n"
            "1. byte [e8-ef] -> 7\n"
            "5. byte [90-90] -> 4\n"
            "7. match! 0\n"
            "4. byte [f0-f0] -> 7\n",
            reverse);

  Dump("[\\x{80}-\\x{10FFFF}]", Regexp::LikePerl, NULL, &reverse);
  EXPECT_EQ("2. byte [80-bf] -> 8\n"
            "8. alt -> 5 | 7\n"
            "5. alt -> 1 | 4\n"
            "7. byte [80-bf] -> 17\n"
            "1. byte [c2-df] -> 18\n"
            "4. byte [a0-bf] -> 3\n"
            "17. alt -> 14 | 16\n"
            "18. match! 0\n"
            "3. byte [e0-e0] -> 18\n"
            "14. alt -> 11 | 13\n"
            "16. byte [80-8f] -> 15\n"
            "11. alt -> 6 | 10\n"
            "13. byte [80-bf] -> 12\n"
            "15. byte [f4-f4] -> 18\n"
            "6. byte [e1-ef] -> 18\n"
            "10. byte [90-bf] -> 9\n"
            "12. byte [f1-f3] -> 18\n"
            "9. byte [f0-f0] -> 18\n",
            reverse);
}

}  // namespace re2
