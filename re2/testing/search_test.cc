// Copyright 2006-2007 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "util/test.h"
#include "re2/prog.h"
#include "re2/regexp.h"
#include "re2/testing/tester.h"
#include "re2/testing/exhaustive_tester.h"

// For target `log' in the Makefile.
#ifndef LOGGING
#define LOGGING 0
#endif

namespace re2 {

struct RegexpTest {
  const char* regexp;
  const char* text;
  const ExpectMatch expect_match;
};

RegexpTest simple_tests[] = {
  { "a", "a", ExpectMatch::Always},
  { "a", "zyzzyva", ExpectMatch::Varies},
  { "a+", "aa", ExpectMatch::Always},
  { "(a+|b)+", "ab", ExpectMatch::Always},
  { "ab|cd", "xabcdx", ExpectMatch::Varies},
  { "h.*od?", "hello\ngoodbye\n", ExpectMatch::Varies},
  { "h.*o", "hello\ngoodbye\n", ExpectMatch::Varies},
  { "h.*o", "goodbye\nhello\n", ExpectMatch::Varies},
  { "h.*o", "hello world", ExpectMatch::Varies},
  { "h.*o", "othello, world", ExpectMatch::Varies},
  { "[^\\s\\S]", "aaaaaaa", ExpectMatch::Never},
  { "a", "aaaaaaa", ExpectMatch::Varies},
  { "a*", "aaaaaaa", ExpectMatch::Always},
  { "a*", "", ExpectMatch::Always},
  { "ab|cd", "xabcdx", ExpectMatch::Varies},
  { "a", "cab", ExpectMatch::Varies},
  { "a*b", "cab", ExpectMatch::Varies},
  { "((((((((((((((((((((x))))))))))))))))))))", "x", ExpectMatch::Always},
  { "[abcd]", "xxxabcdxxx", ExpectMatch::Varies},
  { "[^x]", "xxxabcdxxx", ExpectMatch::Varies},
  { "[abcd]+", "xxxabcdxxx", ExpectMatch::Varies},
  { "[^x]+", "xxxabcdxxx", ExpectMatch::Varies},
  { "(fo|foo)", "fo", ExpectMatch::Always},
  { "(foo|fo)", "foo", ExpectMatch::Always},

  { "aa", "aA", ExpectMatch::Never},
  { "a", "Aa", ExpectMatch::Varies},
  { "a", "A", ExpectMatch::Never},
  { "ABC", "abc", ExpectMatch::Never},
  { "abc", "XABCY", ExpectMatch::Never},
  { "ABC", "xabcy", ExpectMatch::Never},

  // Make sure ^ and $ work.
  // The pathological cases didn't work
  // in the original grep code.
  { "foo|bar|[A-Z]", "foo", ExpectMatch::Always},
  { "^(foo|bar|[A-Z])", "foo", ExpectMatch::Always},
  { "(foo|bar|[A-Z])$", "foo\n", ExpectMatch::Varies},
  { "(foo|bar|[A-Z])$", "foo", ExpectMatch::Always},
  { "^(foo|bar|[A-Z])$", "foo\n", ExpectMatch::Varies},
  { "^(foo|bar|[A-Z])$", "foo", ExpectMatch::Always},
  { "^(foo|bar|[A-Z])$", "bar", ExpectMatch::Always},
  { "^(foo|bar|[A-Z])$", "X", ExpectMatch::Always},
  { "^(foo|bar|[A-Z])$", "XY", ExpectMatch::Never},
  { "^(fo|foo)$", "fo", ExpectMatch::Always},
  { "^(fo|foo)$", "foo", ExpectMatch::Always},
  { "^^(fo|foo)$", "fo", ExpectMatch::Always},
  { "^^(fo|foo)$", "foo", ExpectMatch::Always},
  { "^$", "", ExpectMatch::Always},
  { "^$", "x", ExpectMatch::Never},
  { "^^$", "", ExpectMatch::Always},
  { "^$$", "", ExpectMatch::Always},
  { "^^$", "x", ExpectMatch::Never},
  { "^$$", "x", ExpectMatch::Never},
  { "^^$$", "", ExpectMatch::Always},
  { "^^$$", "x", ExpectMatch::Never},
  { "^^^^^^^^$$$$$$$$", "", ExpectMatch::Always},
  { "^", "x", ExpectMatch::Varies},
  { "$", "x", ExpectMatch::Varies},

  // Word boundaries.
  { "\\bfoo\\b", "nofoo foo that", ExpectMatch::Varies},
  { "a\\b", "faoa x", ExpectMatch::Varies},
  { "\\bbar", "bar x", ExpectMatch::Varies},
  { "\\bbar", "foo\nbar x", ExpectMatch::Varies},
  { "bar\\b", "foobar", ExpectMatch::Varies},
  { "bar\\b", "foobar\nxxx", ExpectMatch::Varies},
  { "(foo|bar|[A-Z])\\b", "foo", ExpectMatch::Always},
  { "(foo|bar|[A-Z])\\b", "foo\n", ExpectMatch::Varies},
  { "\\b", "", ExpectMatch::Never},
  { "\\b", "x", ExpectMatch::Varies},
  { "\\b(foo|bar|[A-Z])", "foo", ExpectMatch::Always},
  { "\\b(foo|bar|[A-Z])\\b", "X", ExpectMatch::Always},
  { "\\b(foo|bar|[A-Z])\\b", "XY", ExpectMatch::Never},
  { "\\b(foo|bar|[A-Z])\\b", "bar", ExpectMatch::Always},
  { "\\b(foo|bar|[A-Z])\\b", "foo", ExpectMatch::Always},
  { "\\b(foo|bar|[A-Z])\\b", "foo\n", ExpectMatch::Varies},
  { "\\b(foo|bar|[A-Z])\\b", "ffoo bbar N x", ExpectMatch::Varies},
  { "\\b(fo|foo)\\b", "fo", ExpectMatch::Always},
  { "\\b(fo|foo)\\b", "foo", ExpectMatch::Always},
  { "\\b\\b", "", ExpectMatch::Never},
  { "\\b\\b", "x", ExpectMatch::Varies},
  { "\\b$", "", ExpectMatch::Never},
  { "\\b$", "x", ExpectMatch::Varies},
  { "\\b$", "y x", ExpectMatch::Varies},
  { "\\b.$", "x", ExpectMatch::Always},
  { "^\\b(fo|foo)\\b", "fo", ExpectMatch::Always},
  { "^\\b(fo|foo)\\b", "foo", ExpectMatch::Always},
  { "^\\b", "", ExpectMatch::Never},
  { "^\\b", "x", ExpectMatch::Varies},
  { "^\\b\\b", "", ExpectMatch::Never},
  { "^\\b\\b", "x", ExpectMatch::Varies},
  { "^\\b$", "", ExpectMatch::Never},
  { "^\\b$", "x", ExpectMatch::Never},
  { "^\\b.$", "x", ExpectMatch::Always},
  { "^\\b.\\b$", "x", ExpectMatch::Always},
  { "^^^^^^^^\\b$$$$$$$", "", ExpectMatch::Never},
  { "^^^^^^^^\\b.$$$$$$", "x", ExpectMatch::Always},
  { "^^^^^^^^\\b$$$$$$$", "x", ExpectMatch::Never},

  // Non-word boundaries.
  { "\\Bfoo\\B", "n foo xfoox that", ExpectMatch::Varies},
  { "a\\B", "faoa x", ExpectMatch::Varies},
  { "\\Bbar", "bar x", ExpectMatch::Never},
  { "\\Bbar", "foo\nbar x", ExpectMatch::Never},
  { "bar\\B", "foobar", ExpectMatch::Never},
  { "bar\\B", "foobar\nxxx", ExpectMatch::Never},
  { "(foo|bar|[A-Z])\\B", "foox", ExpectMatch::Varies},
  { "(foo|bar|[A-Z])\\B", "foo\n", ExpectMatch::Never},
  { "\\B", "", ExpectMatch::Always},
  { "\\B", "x", ExpectMatch::Never},
  { "\\B(foo|bar|[A-Z])", "foo", ExpectMatch::Never},
  { "\\B(foo|bar|[A-Z])\\B", "xXy", ExpectMatch::Varies},
  { "\\B(foo|bar|[A-Z])\\B", "XY", ExpectMatch::Never},
  { "\\B(foo|bar|[A-Z])\\B", "XYZ", ExpectMatch::Varies},
  { "\\B(foo|bar|[A-Z])\\B", "abara", ExpectMatch::Varies},
  { "\\B(foo|bar|[A-Z])\\B", "xfoo_", ExpectMatch::Varies},
  { "\\B(foo|bar|[A-Z])\\B", "xfoo\n", ExpectMatch::Never},
  { "\\B(foo|bar|[A-Z])\\B", "foo bar vNx", ExpectMatch::Varies},
  { "\\B(fo|foo)\\B", "xfoo", ExpectMatch::Varies},
  { "\\B(foo|fo)\\B", "xfooo", ExpectMatch::Varies},
  { "\\B\\B", "", ExpectMatch::Always},
  { "\\B\\B", "x", ExpectMatch::Never},
  { "\\B$", "", ExpectMatch::Always},
  { "\\B$", "x", ExpectMatch::Never},
  { "\\B$", "y x", ExpectMatch::Never},
  { "\\B.$", "x", ExpectMatch::Never},
  { "^\\B(fo|foo)\\B", "fo", ExpectMatch::Never},
  { "^\\B(fo|foo)\\B", "foo", ExpectMatch::Never},
  { "^\\B", "", ExpectMatch::Always},
  { "^\\B", "x", ExpectMatch::Never},
  { "^\\B\\B", "", ExpectMatch::Always},
  { "^\\B\\B", "x", ExpectMatch::Never},
  { "^\\B$", "", ExpectMatch::Always},
  { "^\\B$", "x", ExpectMatch::Never},
  { "^\\B.$", "x", ExpectMatch::Never},
  { "^\\B.\\B$", "x", ExpectMatch::Never},
  { "^^^^^^^^\\B$$$$$$$", "", ExpectMatch::Always},
  { "^^^^^^^^\\B.$$$$$$", "x", ExpectMatch::Never},
  { "^^^^^^^^\\B$$$$$$$", "x", ExpectMatch::Never},

  // PCRE uses only ASCII for \\b computation.
  // All non-ASCII are *not* word characters.
  { "\\bx\\b", "x", ExpectMatch::Always},
  { "\\bx\\b", "x>", ExpectMatch::Varies},
  { "\\bx\\b", "<x", ExpectMatch::Varies},
  { "\\bx\\b", "<x>", ExpectMatch::Varies},
  { "\\bx\\b", "ax", ExpectMatch::Never},
  { "\\bx\\b", "xb", ExpectMatch::Never},
  { "\\bx\\b", "axb", ExpectMatch::Never},
  { "\\bx\\b", "«x", ExpectMatch::Varies},
  { "\\bx\\b", "x»", ExpectMatch::Varies},
  { "\\bx\\b", "«x»", ExpectMatch::Varies},
  { "\\bx\\b", "axb", ExpectMatch::Never},
  { "\\bx\\b", "áxβ", ExpectMatch::Varies},
  { "\\Bx\\B", "axb", ExpectMatch::Varies},
  { "\\Bx\\B", "áxβ", ExpectMatch::Never},

  // Weird boundary cases.
  { "^$^$", "", ExpectMatch::Always},
  { "^$^", "", ExpectMatch::Always},
  { "$^$", "", ExpectMatch::Always},

  { "^$^$", "x", ExpectMatch::Never},
  { "^$^", "x", ExpectMatch::Never},
  { "$^$", "x", ExpectMatch::Never},

  { "^$^$", "x\ny", ExpectMatch::Varies},
  { "^$^", "x\ny", ExpectMatch::Varies},
  { "$^$", "x\ny", ExpectMatch::Varies},

  { "^$^$", "x\n\ny", ExpectMatch::Varies},
  { "^$^", "x\n\ny", ExpectMatch::Varies},
  { "$^$", "x\n\ny", ExpectMatch::Varies},

  { "^(foo\\$)$", "foo$bar", ExpectMatch::Never},
  { "(foo\\$)", "foo$bar", ExpectMatch::Varies},
  { "^...$", "abc", ExpectMatch::Always},

  // UTF-8
  { "^\xe6\x9c\xac$", "\xe6\x9c\xac", ExpectMatch::Always },
  { "^...$", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", ExpectMatch::Varies },
  { "^...$", ".\xe6\x9c\xac.", ExpectMatch::Varies },

  { "^\\C\\C\\C$", "\xe6\x9c\xac", ExpectMatch::Always },
  { "^\\C$", "\xe6\x9c\xac", ExpectMatch::Never },
  { "^\\C\\C\\C$", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", ExpectMatch::Never },

  // Latin1
  { "^...$", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", ExpectMatch::Varies },
  { "^.........$", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", ExpectMatch::Varies },
  { "^...$", ".\xe6\x9c\xac.", ExpectMatch::Varies },
  { "^.....$", ".\xe6\x9c\xac.", ExpectMatch::Varies },

  // Perl v Posix
  { "\\B(fo|foo)\\B", "xfooo", ExpectMatch::Varies },
  { "(fo|foo)", "foo", ExpectMatch::Always },

  // Octal escapes.
  { "\\141", "a", ExpectMatch::Always },
  { "\\060", "0", ExpectMatch::Always },
  { "\\0600", "00", ExpectMatch::Always },
  { "\\608", "08", ExpectMatch::Always },
  { "\\01", "\01", ExpectMatch::Always },
  { "\\018", "\01" "8", ExpectMatch::Always },

  // Hexadecimal escapes
  { "\\x{61}", "a", ExpectMatch::Always },
  { "\\x61", "a", ExpectMatch::Always },
  { "\\x{00000061}", "a", ExpectMatch::Always },

  // Unicode scripts.
  { "\\p{Greek}+", "aαβb", ExpectMatch::Varies },
  { "\\P{Greek}+", "aαβb", ExpectMatch::Varies },
  { "\\p{^Greek}+", "aαβb", ExpectMatch::Varies },
  { "\\P{^Greek}+", "aαβb", ExpectMatch::Varies },

  // Unicode properties.  Nd is decimal number.  N is any number.
  { "[^0-9]+",  "abc123", ExpectMatch::Varies },
  { "\\p{Nd}+", "abc123²³¼½¾₀₉", ExpectMatch::Varies },
  { "\\p{^Nd}+", "abc123²³¼½¾₀₉", ExpectMatch::Varies },
  { "\\P{Nd}+", "abc123²³¼½¾₀₉", ExpectMatch::Varies },
  { "\\P{^Nd}+", "abc123²³¼½¾₀₉", ExpectMatch::Varies },
  { "\\pN+", "abc123²³¼½¾₀₉", ExpectMatch::Varies },
  { "\\p{N}+", "abc123²³¼½¾₀₉", ExpectMatch::Varies },
  { "\\p{^N}+", "abc123²³¼½¾₀₉", ExpectMatch::Varies },

  { "\\p{Any}+", "abc123", ExpectMatch::Always },

  // Character classes & case folding.
  { "(?i)[@-A]+", "@AaB", ExpectMatch::Varies },  // matches @Aa but not B
  { "(?i)[A-Z]+", "aAzZ", ExpectMatch::Always },
  { "(?i)[^\\\\]+", "Aa\\", ExpectMatch::Varies },  // \\ is between A-Z and a-z -
                               // splits the ranges in an interesting way.

  // would like to use, but PCRE mishandles in full-match, non-greedy mode
  // { "(?i)[\\\\]+", "Aa" },

  { "(?i)[acegikmoqsuwy]+", "acegikmoqsuwyACEGIKMOQSUWY", ExpectMatch::Always },

  // Character classes & case folding.
  { "[@-A]+", "@AaB", ExpectMatch::Varies },
  { "[A-Z]+", "aAzZ", ExpectMatch::Varies },
  { "[^\\\\]+", "Aa\\", ExpectMatch::Varies },
  { "[acegikmoqsuwy]+", "acegikmoqsuwyACEGIKMOQSUWY", ExpectMatch::Varies },

  // Anchoring.  (^abc in aabcdef was a former bug)
  // The tester checks for a match in the text and
  // subpieces of the text with a byte removed on either side.
  { "^abc", "abcdef", ExpectMatch::Varies },
  { "^abc", "aabcdef", ExpectMatch::Never },
  { "^[ay]*[bx]+c", "abcdef", ExpectMatch::Varies },
  { "^[ay]*[bx]+c", "aabcdef", ExpectMatch::Varies },
  { "def$", "abcdef", ExpectMatch::Varies },
  { "def$", "abcdeff", ExpectMatch::Never },
  { "d[ex][fy]$", "abcdef", ExpectMatch::Varies },
  { "d[ex][fy]$", "abcdeff", ExpectMatch::Never },
  { "[dz][ex][fy]$", "abcdef", ExpectMatch::Varies },
  { "[dz][ex][fy]$", "abcdeff", ExpectMatch::Never },
  { "(?m)^abc", "abcdef", ExpectMatch::Varies },
  { "(?m)^abc", "aabcdef", ExpectMatch::Never },
  { "(?m)^[ay]*[bx]+c", "abcdef", ExpectMatch::Varies },
  { "(?m)^[ay]*[bx]+c", "aabcdef", ExpectMatch::Varies },
  { "(?m)def$", "abcdef", ExpectMatch::Varies },
  { "(?m)def$", "abcdeff", ExpectMatch::Never },
  { "(?m)d[ex][fy]$", "abcdef", ExpectMatch::Varies },
  { "(?m)d[ex][fy]$", "abcdeff", ExpectMatch::Never },
  { "(?m)[dz][ex][fy]$", "abcdef", ExpectMatch::Varies },
  { "(?m)[dz][ex][fy]$", "abcdeff", ExpectMatch::Never },
  { "^", "a", ExpectMatch::Varies },
  { "^^", "a", ExpectMatch::Varies },

  // Context.
  // The tester checks for a match in the text and
  // subpieces of the text with a byte removed on either side.
  { "a", "a", ExpectMatch::Always },
  { "ab*", "a", ExpectMatch::Always },
  { "a\\C*", "a", ExpectMatch::Always },
  { "a\\C+", "a", ExpectMatch::Never },
  { "a\\C?", "a", ExpectMatch::Always },
  { "a\\C*?", "a", ExpectMatch::Always },
  { "a\\C+?", "a", ExpectMatch::Never },
  { "a\\C??", "a", ExpectMatch::Always },

  // Former bugs.
  { "a\\C*|ba\\C", "baba", ExpectMatch::Varies },
};

TEST(Regexp, SearchTests) {
  int failures = 0;
  for (int i = 0; i < arraysize(simple_tests); i++) {
    const RegexpTest& t = simple_tests[i];
      if (!TestRegexpOnText(t.regexp, t.text, t.expect_match))
      failures++;

    if (LOGGING) {
      // Build a dummy ExhaustiveTest call that will trigger just
      // this one test, so that we log the test case.
      std::vector<string> atom, alpha, ops;
      atom.push_back(t.regexp);
      alpha.push_back(t.text);
      ExhaustiveTest(1, 0, atom, ops, 1, alpha, "", "");
    }
  }
  EXPECT_EQ(failures, 0);
}

}  // namespace re2
