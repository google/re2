// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <string>

#include "util/test.h"
#include "util/logging.h"
#include "re2/prog.h"
#include "re2/regexp.h"

namespace re2 {

struct PrefixTest {
  const char* regexp;
  bool return_value;
  const char* prefix;
  bool foldcase;
  const char* suffix;
};

static PrefixTest tests[] = {
  // Empty cases.
  { "", false },
  { "(?m)^", false },
  { "(?-m)^", false },

  // If the regexp has no ^, there's no required prefix.
  { "abc", false },

  // If the regexp immediately goes into
  // something not a literal match, there's no required prefix.
  { "^(abc)", false },
  { "^a*",  false },

  // Otherwise, it should work.
  { "^abc$", true, "abc", false, "(?-m:$)" },
  { "^abc", true, "abc", false, "" },
  { "^(?i)abc", true, "abc", true, "" },
  { "^abcd*", true, "abc", false, "d*" },
  { "^[Aa][Bb]cd*", true, "ab", true, "cd*" },
  { "^ab[Cc]d*", true, "ab", false, "[Cc]d*" },
  { "^☺abc", true, "☺abc", false, "" },
};

TEST(RequiredPrefix, SimpleTests) {
  for (size_t i = 0; i < arraysize(tests); i++) {
    const PrefixTest& t = tests[i];
    for (size_t j = 0; j < 2; j++) {
      Regexp::ParseFlags flags = Regexp::LikePerl;
      if (j == 0)
        flags = flags | Regexp::Latin1;
      Regexp* re = Regexp::Parse(t.regexp, flags, NULL);
      ASSERT_TRUE(re != NULL) << " " << t.regexp;

      std::string p;
      bool f;
      Regexp* s;
      ASSERT_EQ(t.return_value, re->RequiredPrefix(&p, &f, &s))
        << " " << t.regexp << " " << (j == 0 ? "latin1" : "utf8")
        << " " << re->Dump();
      if (t.return_value) {
        ASSERT_EQ(p, std::string(t.prefix))
          << " " << t.regexp << " " << (j == 0 ? "latin1" : "utf8");
        ASSERT_EQ(f, t.foldcase)
          << " " << t.regexp << " " << (j == 0 ? "latin1" : "utf8");
        ASSERT_EQ(s->ToString(), std::string(t.suffix))
          << " " << t.regexp << " " << (j == 0 ? "latin1" : "utf8");
        s->Decref();
      }
      re->Decref();
    }
  }
}

static PrefixTest for_accel_tests[] = {
  // Empty cases.
  { "", false },
  { "(?m)^", false },
  { "(?-m)^", false },

  // If the regexp has a ^, there's no required prefix.
  { "^abc", false },

  // If the regexp immediately goes into
  // something not a literal match, there's no required prefix.
  { "(abc)", false },
  { "a*",  false },

  // Otherwise, it should work.
  { "abc$", true, "abc", false, },
  { "abc", true, "abc", false, },
  { "(?i)abc", true, "abc", true, },
  { "abcd*", true, "abc", false, },
  { "[Aa][Bb]cd*", true, "ab", true, },
  { "ab[Cc]d*", true, "ab", false, },
  { "☺abc", true, "☺abc", false, },
};

TEST(RequiredPrefixForAccel, SimpleTests) {
  for (size_t i = 0; i < arraysize(for_accel_tests); i++) {
    const PrefixTest& t = for_accel_tests[i];
    for (size_t j = 0; j < 2; j++) {
      Regexp::ParseFlags flags = Regexp::LikePerl;
      if (j == 0)
        flags = flags | Regexp::Latin1;
      Regexp* re = Regexp::Parse(t.regexp, flags, NULL);
      ASSERT_TRUE(re != NULL) << " " << t.regexp;

      std::string p;
      bool f;
      ASSERT_EQ(t.return_value, re->RequiredPrefixForAccel(&p, &f))
        << " " << t.regexp << " " << (j == 0 ? "latin1" : "utf8")
        << " " << re->Dump();
      if (t.return_value) {
        ASSERT_EQ(p, std::string(t.prefix))
          << " " << t.regexp << " " << (j == 0 ? "latin1" : "utf8");
        ASSERT_EQ(f, t.foldcase)
          << " " << t.regexp << " " << (j == 0 ? "latin1" : "utf8");
      }
      re->Decref();
    }
  }
}

TEST(PrefixAccel, BasicTest) {
  Regexp* re = Regexp::Parse("abc\\d+", Regexp::LikePerl, NULL);
  ASSERT_TRUE(re != NULL);
  Prog* prog = re->CompileToProg(0);
  ASSERT_TRUE(prog != NULL);
  for (int i = 0; i < 100; i++) {
    std::string text(i, 'a');
    const char* p = reinterpret_cast<const char*>(
        prog->PrefixAccel(text.data(), text.size()));
    EXPECT_TRUE(p == NULL);
    text.append("abc");
    p = reinterpret_cast<const char*>(
        prog->PrefixAccel(text.data(), text.size()));
    EXPECT_EQ(i, p-text.data());
  }
  delete prog;
  re->Decref();
}

}  // namespace re2
