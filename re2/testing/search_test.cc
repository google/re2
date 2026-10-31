// Copyright 2006-2007 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "absl/base/macros.h"
#include "absl/strings/string_view.h"
#include "gtest/gtest.h"
#include "re2/prog.h"
#include "re2/re2.h"
#include "re2/regexp.h"
#include "re2/testing/exhaustive_tester.h"
#include "re2/testing/tester.h"
#include "util/malloc_counter.h"

// For target `log' in the Makefile.
#ifndef LOGGING
#define LOGGING 0
#endif

namespace re2 {

struct RegexpTest {
  const char* regexp;
  const char* text;
};

RegexpTest simple_tests[] = {
  { "a", "a" },
  { "a", "zyzzyva" },
  { "a+", "aa" },
  { "(a+|b)+", "ab" },
  { "ab|cd", "xabcdx" },
  { "h.*od?", "hello\ngoodbye\n" },
  { "h.*o", "hello\ngoodbye\n" },
  { "h.*o", "goodbye\nhello\n" },
  { "h.*o", "hello world" },
  { "h.*o", "othello, world" },
  { "[^\\s\\S]", "aaaaaaa" },
  { "a", "aaaaaaa" },
  { "a*", "aaaaaaa" },
  { "a*", "" },
  { "ab|cd", "xabcdx" },
  { "a", "cab" },
  { "a*b", "cab" },
  { "((((((((((((((((((((x))))))))))))))))))))", "x" },
  { "[abcd]", "xxxabcdxxx" },
  { "[^x]", "xxxabcdxxx" },
  { "[abcd]+", "xxxabcdxxx" },
  { "[^x]+", "xxxabcdxxx" },
  { "(fo|foo)", "fo" },
  { "(foo|fo)", "foo" },

  { "aa", "aA" },
  { "a", "Aa" },
  { "a", "A" },
  { "ABC", "abc" },
  { "abc", "XABCY" },
  { "ABC", "xabcy" },

  // Make sure ^ and $ work.
  // The pathological cases didn't work
  // in the original grep code.
  { "foo|bar|[A-Z]", "foo" },
  { "^(foo|bar|[A-Z])", "foo" },
  { "(foo|bar|[A-Z])$", "foo\n" },
  { "(foo|bar|[A-Z])$", "foo" },
  { "^(foo|bar|[A-Z])$", "foo\n" },
  { "^(foo|bar|[A-Z])$", "foo" },
  { "^(foo|bar|[A-Z])$", "bar" },
  { "^(foo|bar|[A-Z])$", "X" },
  { "^(foo|bar|[A-Z])$", "XY" },
  { "^(fo|foo)$", "fo" },
  { "^(fo|foo)$", "foo" },
  { "^^(fo|foo)$", "fo" },
  { "^^(fo|foo)$", "foo" },
  { "^$", "" },
  { "^$", "x" },
  { "^^$", "" },
  { "^$$", "" },
  { "^^$", "x" },
  { "^$$", "x" },
  { "^^$$", "" },
  { "^^$$", "x" },
  { "^^^^^^^^$$$$$$$$", "" },
  { "^", "x" },
  { "$", "x" },

  // Word boundaries.
  { "\\bfoo\\b", "nofoo foo that" },
  { "a\\b", "faoa x" },
  { "\\bbar", "bar x" },
  { "\\bbar", "foo\nbar x" },
  { "bar\\b", "foobar" },
  { "bar\\b", "foobar\nxxx" },
  { "(foo|bar|[A-Z])\\b", "foo" },
  { "(foo|bar|[A-Z])\\b", "foo\n" },
  { "\\b", "" },
  { "\\b", "x" },
  { "\\b(foo|bar|[A-Z])", "foo" },
  { "\\b(foo|bar|[A-Z])\\b", "X" },
  { "\\b(foo|bar|[A-Z])\\b", "XY" },
  { "\\b(foo|bar|[A-Z])\\b", "bar" },
  { "\\b(foo|bar|[A-Z])\\b", "foo" },
  { "\\b(foo|bar|[A-Z])\\b", "foo\n" },
  { "\\b(foo|bar|[A-Z])\\b", "ffoo bbar N x" },
  { "\\b(fo|foo)\\b", "fo" },
  { "\\b(fo|foo)\\b", "foo" },
  { "\\b\\b", "" },
  { "\\b\\b", "x" },
  { "\\b$", "" },
  { "\\b$", "x" },
  { "\\b$", "y x" },
  { "\\b.$", "x" },
  { "^\\b(fo|foo)\\b", "fo" },
  { "^\\b(fo|foo)\\b", "foo" },
  { "^\\b", "" },
  { "^\\b", "x" },
  { "^\\b\\b", "" },
  { "^\\b\\b", "x" },
  { "^\\b$", "" },
  { "^\\b$", "x" },
  { "^\\b.$", "x" },
  { "^\\b.\\b$", "x" },
  { "^^^^^^^^\\b$$$$$$$", "" },
  { "^^^^^^^^\\b.$$$$$$", "x" },
  { "^^^^^^^^\\b$$$$$$$", "x" },

  // Non-word boundaries.
  { "\\Bfoo\\B", "n foo xfoox that" },
  { "a\\B", "faoa x" },
  { "\\Bbar", "bar x" },
  { "\\Bbar", "foo\nbar x" },
  { "bar\\B", "foobar" },
  { "bar\\B", "foobar\nxxx" },
  { "(foo|bar|[A-Z])\\B", "foox" },
  { "(foo|bar|[A-Z])\\B", "foo\n" },
  { "\\B", "" },
  { "\\B", "x" },
  { "\\B(foo|bar|[A-Z])", "foo" },
  { "\\B(foo|bar|[A-Z])\\B", "xXy" },
  { "\\B(foo|bar|[A-Z])\\B", "XY" },
  { "\\B(foo|bar|[A-Z])\\B", "XYZ" },
  { "\\B(foo|bar|[A-Z])\\B", "abara" },
  { "\\B(foo|bar|[A-Z])\\B", "xfoo_" },
  { "\\B(foo|bar|[A-Z])\\B", "xfoo\n" },
  { "\\B(foo|bar|[A-Z])\\B", "foo bar vNx" },
  { "\\B(fo|foo)\\B", "xfoo" },
  { "\\B(foo|fo)\\B", "xfooo" },
  { "\\B\\B", "" },
  { "\\B\\B", "x" },
  { "\\B$", "" },
  { "\\B$", "x" },
  { "\\B$", "y x" },
  { "\\B.$", "x" },
  { "^\\B(fo|foo)\\B", "fo" },
  { "^\\B(fo|foo)\\B", "foo" },
  { "^\\B", "" },
  { "^\\B", "x" },
  { "^\\B\\B", "" },
  { "^\\B\\B", "x" },
  { "^\\B$", "" },
  { "^\\B$", "x" },
  { "^\\B.$", "x" },
  { "^\\B.\\B$", "x" },
  { "^^^^^^^^\\B$$$$$$$", "" },
  { "^^^^^^^^\\B.$$$$$$", "x" },
  { "^^^^^^^^\\B$$$$$$$", "x" },

  // PCRE uses only ASCII for \b computation.
  // All non-ASCII are *not* word characters.
  { "\\bx\\b", "x" },
  { "\\bx\\b", "x>" },
  { "\\bx\\b", "<x" },
  { "\\bx\\b", "<x>" },
  { "\\bx\\b", "ax" },
  { "\\bx\\b", "xb" },
  { "\\bx\\b", "axb" },
  { "\\bx\\b", "«x" },
  { "\\bx\\b", "x»" },
  { "\\bx\\b", "«x»" },
  { "\\bx\\b", "axb" },
  { "\\bx\\b", "áxβ" },
  { "\\Bx\\B", "axb" },
  { "\\Bx\\B", "áxβ" },

  // Weird boundary cases.
  { "^$^$", "" },
  { "^$^", "" },
  { "$^$", "" },

  { "^$^$", "x" },
  { "^$^", "x" },
  { "$^$", "x" },

  { "^$^$", "x\ny" },
  { "^$^", "x\ny" },
  { "$^$", "x\ny" },

  { "^$^$", "x\n\ny" },
  { "^$^", "x\n\ny" },
  { "$^$", "x\n\ny" },

  { "^(foo\\$)$", "foo$bar" },
  { "(foo\\$)", "foo$bar" },
  { "^...$", "abc" },

  // UTF-8
  { "^\xe6\x9c\xac$", "\xe6\x9c\xac" },
  { "^...$", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e" },
  { "^...$", ".\xe6\x9c\xac." },

  { "^\\C\\C\\C$", "\xe6\x9c\xac" },
  { "^\\C$", "\xe6\x9c\xac" },
  { "^\\C\\C\\C$", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e" },

  // Latin1
  { "^...$", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e" },
  { "^.........$", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e" },
  { "^...$", ".\xe6\x9c\xac." },
  { "^.....$", ".\xe6\x9c\xac." },

  // Perl v Posix
  { "\\B(fo|foo)\\B", "xfooo" },
  { "(fo|foo)", "foo" },

  // Octal escapes.
  { "\\141", "a" },
  { "\\060", "0" },
  { "\\0600", "00" },
  { "\\608", "08" },
  { "\\01", "\01" },
  { "\\018", "\01" "8" },

  // Hexadecimal escapes
  { "\\x{61}", "a" },
  { "\\x61", "a" },
  { "\\x{00000061}", "a" },

  // Unicode scripts.
  { "\\p{Greek}+", "aαβb" },
  { "\\P{Greek}+", "aαβb" },
  { "\\p{^Greek}+", "aαβb" },
  { "\\P{^Greek}+", "aαβb" },

  // Unicode properties.  Nd is decimal number.  N is any number.
  { "[^0-9]+",  "abc123" },
  { "\\p{Nd}+", "abc123²³¼½¾₀₉" },
  { "\\p{^Nd}+", "abc123²³¼½¾₀₉" },
  { "\\P{Nd}+", "abc123²³¼½¾₀₉" },
  { "\\P{^Nd}+", "abc123²³¼½¾₀₉" },
  { "\\pN+", "abc123²³¼½¾₀₉" },
  { "\\p{N}+", "abc123²³¼½¾₀₉" },
  { "\\p{^N}+", "abc123²³¼½¾₀₉" },

  { "\\p{Any}+", "abc123" },

  // Character classes & case folding.
  { "(?i)[@-A]+", "@AaB" },  // matches @Aa but not B
  { "(?i)[A-Z]+", "aAzZ" },
  { "(?i)[^\\\\]+", "Aa\\" },  // \\ is between A-Z and a-z -
                               // splits the ranges in an interesting way.

  // would like to use, but PCRE mishandles in full-match, non-greedy mode
  // { "(?i)[\\\\]+", "Aa" },

  { "(?i)[acegikmoqsuwy]+", "acegikmoqsuwyACEGIKMOQSUWY" },

  // Character classes & case folding.
  { "[@-A]+", "@AaB" },
  { "[A-Z]+", "aAzZ" },
  { "[^\\\\]+", "Aa\\" },
  { "[acegikmoqsuwy]+", "acegikmoqsuwyACEGIKMOQSUWY" },

  // Anchoring.  (^abc in aabcdef was a former bug)
  // The tester checks for a match in the text and
  // subpieces of the text with a byte removed on either side.
  { "^abc", "abcdef" },
  { "^abc", "aabcdef" },
  { "^[ay]*[bx]+c", "abcdef" },
  { "^[ay]*[bx]+c", "aabcdef" },
  { "def$", "abcdef" },
  { "def$", "abcdeff" },
  { "d[ex][fy]$", "abcdef" },
  { "d[ex][fy]$", "abcdeff" },
  { "[dz][ex][fy]$", "abcdef" },
  { "[dz][ex][fy]$", "abcdeff" },
  { "(?m)^abc", "abcdef" },
  { "(?m)^abc", "aabcdef" },
  { "(?m)^[ay]*[bx]+c", "abcdef" },
  { "(?m)^[ay]*[bx]+c", "aabcdef" },
  { "(?m)def$", "abcdef" },
  { "(?m)def$", "abcdeff" },
  { "(?m)d[ex][fy]$", "abcdef" },
  { "(?m)d[ex][fy]$", "abcdeff" },
  { "(?m)[dz][ex][fy]$", "abcdef" },
  { "(?m)[dz][ex][fy]$", "abcdeff" },
  { "^", "a" },
  { "^^", "a" },

  // Context.
  // The tester checks for a match in the text and
  // subpieces of the text with a byte removed on either side.
  { "a", "a" },
  { "ab*", "a" },
  { "a\\C*", "a" },
  { "a\\C+", "a" },
  { "a\\C?", "a" },
  { "a\\C*?", "a" },
  { "a\\C+?", "a" },
  { "a\\C??", "a" },

  // Former bugs.
  { "a\\C*|ba\\C", "baba" },
  { "\\w*I\\w*", "Inc." },
  { "(?:|a)*", "aaa" },
  { "(?:|a)+", "aaa" },
};

TEST(Regexp, SearchTests) {
  int failures = 0;
  for (size_t i = 0; i < ABSL_ARRAYSIZE(simple_tests); i++) {
    const RegexpTest& t = simple_tests[i];
    if (!TestRegexpOnText(t.regexp, t.text))
      failures++;

    if (LOGGING) {
      // Build a dummy ExhaustiveTest call that will trigger just
      // this one test, so that we log the test case.
      std::vector<std::string> atom, alpha, ops;
      atom.push_back(t.regexp);
      alpha.push_back(t.text);
      ExhaustiveTest(1, 0, atom, ops, 1, alpha, "", "");
    }
  }
  EXPECT_EQ(failures, 0);
}

// One-pass regexps with capturing groups, and texts to search with them.
// IsOnePass() cuts the one-pass NFA's memory from the DFA budget; it keeps a
// small NFA, and SearchOnePass() builds a bigger one on first use. The last
// regexp's NFA is well above Prog::kMaxEagerOnePassBytes, so it is built
// lazily.
struct OnePassTest {
  const char* regexp;
  const char* texts[6];
};

static const OnePassTest onepass_tests[] = {
  { "^/home/([^/]+)/(src|old-src)/([^/]+)/bin",
    { "/home/xx/src/proj-1/bin", "/home/xx/old-src/p/bin",
      "/home/xx/src/p/binx", "/home//src/p/bin", "/home/xx/src/p", "" } },
  { "(\\d+)-(\\d+)",
    { "12-345", "12-", "a12-3", "12-34x", "0-0", "" } },
  { "(?i)([a-z]{2})-([a-z0-9]+)-(live|test)",
    { "us-east1-live", "US-East1-Test", "us--live", "us-east1-dev",
      "u-east1-live", "" } },
  { "^web-(backend|frontend|mixer)-(\\d{4})$",
    { "web-mixer-0003", "web-mixer-003", "web-mixer-00031",
      "web-indexer-0003", "web-backend-1234", "" } },
  { "^(apple|apricot|banana|blueberry|cherry|cranberry|date|elderberry|fig|"
    "grape|guava|kiwi|lemon|lime|mango|melon|nectarine|orange|papaya|peach|"
    "pear|plum|quince|raspberry|strawberry|tangerine|watermelon)-([a-z]+)-"
    "(\\d+)$",
    { "watermelon-east-42", "kiwi-x-1", "kiwi--1", "pineapple-x-1",
      "strawberry-west-007x", "" } },
};

// Checks that the DFA budget is cut once, by IsOnePass(), and that the
// one-pass NFA that IsOnePass() and the first SearchOnePass() build together
// takes exactly the memory that was cut.
TEST(OnePass, Budget) {
  for (const OnePassTest& t : onepass_tests) {
    Regexp* re = Regexp::Parse(t.regexp, Regexp::LikePerl, NULL);
    ASSERT_TRUE(re != NULL);
    Prog* prog = re->CompileToProg(0);
    ASSERT_TRUE(prog != NULL);
    int nsub = 1 + re->NumCaptures();
    int64_t budget = prog->dfa_mem();
    int64_t onepass_budget;
    int64_t kept_by_isonepass;
    int64_t usage;
    {
      testing::MallocCounter m(testing::MallocCounter::THIS_THREAD_ONLY);
      ASSERT_TRUE(prog->IsOnePass()) << t.regexp;
      kept_by_isonepass = m.HeapGrowth();
      onepass_budget = prog->dfa_mem();
      ASSERT_LT(onepass_budget, budget) << t.regexp;
      // A second IsOnePass() gives the same answer and does not cut the
      // budget again.
      ASSERT_TRUE(prog->IsOnePass()) << t.regexp;
      ASSERT_EQ(prog->dfa_mem(), onepass_budget) << t.regexp;
      // Nor does the first SearchOnePass().
      absl::string_view text(t.texts[0]);
      absl::string_view sub[5];
      ASSERT_TRUE(prog->SearchOnePass(text, text, Prog::kAnchored,
                                      Prog::kFullMatch, sub, nsub))
          << t.regexp;
      ASSERT_EQ(prog->dfa_mem(), onepass_budget) << t.regexp;
      usage = m.HeapGrowth();
    }
    // The open-source MallocCounter is a stub that always returns 0.
    if (usage != 0) {
      int64_t nbytes = budget - onepass_budget;
      ASSERT_EQ(usage, nbytes) << t.regexp;
      // IsOnePass() keeps a small NFA; SearchOnePass() builds a bigger one.
      ASSERT_EQ(kept_by_isonepass,
                nbytes <= Prog::kMaxEagerOnePassBytes ? nbytes : 0)
          << t.regexp;
    }
    delete prog;
    re->Decref();
  }

  // A program that is not one-pass keeps its whole budget.
  Regexp* re = Regexp::Parse("(a*)(a*)b", Regexp::LikePerl, NULL);
  ASSERT_TRUE(re != NULL);
  Prog* prog = re->CompileToProg(0);
  ASSERT_TRUE(prog != NULL);
  int64_t budget = prog->dfa_mem();
  ASSERT_FALSE(prog->IsOnePass());
  ASSERT_FALSE(prog->IsOnePass());
  ASSERT_EQ(prog->dfa_mem(), budget);
  delete prog;
  re->Decref();
}

// Compares SearchOnePass() with SearchNFA() on the first call, which builds
// the one-pass NFA, and on later calls.
TEST(OnePass, SameAsNFA) {
  int nfail = 0;
  for (const OnePassTest& t : onepass_tests) {
    Regexp* re = Regexp::Parse(t.regexp, Regexp::LikePerl, NULL);
    ASSERT_TRUE(re != NULL);
    int nsub = 1 + re->NumCaptures();
    for (Prog::MatchKind kind : {Prog::kFirstMatch, Prog::kFullMatch}) {
      Prog* prog = re->CompileToProg(0);
      ASSERT_TRUE(prog != NULL);
      ASSERT_TRUE(prog->IsOnePass()) << t.regexp;
      for (int pass = 0; pass < 3; pass++) {
        for (const char* p : t.texts) {
          absl::string_view text(p);
          absl::string_view onepass[5];
          absl::string_view nfa[5];
          bool onepass_matched = prog->SearchOnePass(
              text, text, Prog::kAnchored, kind, onepass, nsub);
          bool nfa_matched =
              prog->SearchNFA(text, text, Prog::kAnchored, kind, nfa, nsub);
          bool same = onepass_matched == nfa_matched;
          for (int i = 0; same && nfa_matched && i < nsub; i++) {
            same = onepass[i].data() == nfa[i].data() &&
                   onepass[i].size() == nfa[i].size();
          }
          if (!same) {
            ADD_FAILURE() << t.regexp << " on \"" << text << "\", kind "
                          << kind << ", pass " << pass
                          << ": SearchOnePass and SearchNFA differ";
            nfail++;
          }
        }
      }
      delete prog;
    }
    re->Decref();
  }
  EXPECT_EQ(nfail, 0);
}

// Releases many threads at once on each freshly built RE2, so that they race
// to run its first one-pass match (FullMatch with captures on a short text,
// which skips the DFA) and so to build the one-pass NFA.
TEST(Multithreaded, OnePassFirstUse) {
  struct Case {
    const char* regexp;  // one-pass, as OnePass.Budget checks
    const char* text;
    const char* sub1;
    const char* sub2;
  };
  static const Case cases[] = {
    { "^/home/([^/]+)/(src|old-src)/[^/]+/bin", "/home/xx/old-src/p/bin",
      "xx", "old-src" },
    { "(\\d+)-(\\d+)", "12-345", "12", "345" },
    { "(?i)([a-z]{2})-[a-z0-9]+-(live|test)", "US-East1-Test", "US", "Test" },
    { "^(apple|apricot|banana|blueberry|cherry|cranberry|date|elderberry|fig|"
      "grape|guava|kiwi|lemon|lime|mango|melon|nectarine|orange|papaya|peach|"
      "pear|plum|quince|raspberry|strawberry|tangerine|watermelon)-([a-z]+)$",
      "melon-east", "melon", "east" },
  };
  const int kThreads = 16;
  const int kRounds = 20;
  for (const Case& c : cases) {
    for (int round = 0; round < kRounds; round++) {
      RE2 re(c.regexp);
      ASSERT_TRUE(re.ok()) << c.regexp;
      std::atomic<bool> go(false);
      std::atomic<int> failures(0);
      std::vector<std::thread> threads;
      for (int i = 0; i < kThreads; i++) {
        threads.emplace_back([&]() {
          while (!go.load(std::memory_order_acquire))
            std::this_thread::yield();
          absl::string_view sub1, sub2;
          if (!RE2::FullMatch(c.text, re, &sub1, &sub2) || sub1 != c.sub1 ||
              sub2 != c.sub2)
            failures.fetch_add(1);
        });
      }
      go.store(true, std::memory_order_release);
      for (std::thread& t : threads)
        t.join();
      ASSERT_EQ(failures.load(), 0) << c.regexp;
    }
  }
}

}  // namespace re2
