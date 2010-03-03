// Copyright 2008 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Exhaustive testing of regular expression matching.

// Each test picks an alphabet (e.g., "abc"), a maximum string length,
// a maximum regular expression length, and a maximum number of letters
// that can appear in the regular expression.  Given these parameters,
// it tries every possible regular expression and string, verifying that
// the NFA, DFA, and a trivial backtracking implementation agree about
// the location of the match.

#include <stdlib.h>
#include <stdio.h>

#include "util/test.h"
#include "re2/testing/exhaustive_tester.h"
#include "re2/testing/tester.h"

DEFINE_bool(show_regexps, false, "show regexps during testing");

DEFINE_int32(max_bad_regexp_inputs, 1,
             "Stop testing a regular expression after finding this many "
             "strings that break it.");

// Compiled in debug mode, the usual tests run for over an hour.
// Have to cut it down to make the unit test machines happy.
DEFINE_bool(quick_debug_mode, true, "Run fewer tests in debug mode.");

namespace re2 {

// Processes a single generated regexp.
// Compiles it using Regexp interface and PCRE, and then
// checks that NFA, DFA, and PCRE all return the same results.
void ExhaustiveTester::HandleRegexp(const string& const_regexp) {
  regexps_++;
  
  string regexp = const_regexp;
  if (!topwrapper_.empty())
    regexp = StringPrintf(topwrapper_.c_str(), regexp.c_str());

  if (FLAGS_show_regexps) {
    printf("\r%s", regexp.c_str());
    fflush(stdout);
  }

  Tester tester(regexp);
  if (tester.error())
    return;

  strgen_.Reset();
  strgen_.GenerateNULL();
  if (randomstrings_)
    strgen_.Random(stringseed_, stringcount_);
  int bad_inputs = 0;
  while (strgen_.HasNext()) {
    tests_++;
    if (!tester.TestInput(strgen_.Next())) {
      failures_++;
      if (++bad_inputs >= FLAGS_max_bad_regexp_inputs)
        break;
    }
  }
}

// Runs an exhaustive test on the given parameters.
void ExhaustiveTest(int maxatoms, int maxops,
                    const vector<string>& alphabet,
                    const vector<string>& ops,
                    int maxstrlen, const vector<string>& stralphabet,
                    const string& wrapper,
                    const string& topwrapper) {
  if (DEBUG_MODE && FLAGS_quick_debug_mode) {
    if (maxatoms > 1)
      maxatoms--;
    if (maxops > 1)
      maxops--;
    if (maxstrlen > 1)
      maxstrlen--;
  }
  ExhaustiveTester t(maxatoms, maxops, alphabet, ops,
                     maxstrlen, stralphabet, wrapper,
                     topwrapper);
  t.Generate();
  printf("%d regexps, %d tests, %d failures [%d/%d str]\n",
         t.regexps(), t.tests(), t.failures(), maxstrlen, (int)stralphabet.size());
  EXPECT_EQ(0, t.failures());
}

// Runs an exhaustive test using the given parameters and
// the basic egrep operators.
void EgrepTest(int maxatoms, int maxops, const string& alphabet,
               int maxstrlen, const string& stralphabet,
               const string& wrapper) {
  const char* tops[] = { "", "^(?:%s)", "(?:%s)$", "^(?:%s)$" };

  for (int i = 0; i < arraysize(tops); i++) {
    ExhaustiveTest(maxatoms, maxops,
                   Split("", alphabet),
                   RegexpGenerator::EgrepOps(),
                   maxstrlen,
                   Split("", stralphabet),
                   wrapper,
                   tops[i]);
  }
}

}  // namespace re2
