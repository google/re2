// Copyright 2008 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Exhaustive testing of regular expression matching.

#include <string>
#include <vector>

<<<<<<< HEAD   (edf7fc Move util/flags.h into the testing target.)
#include "gtest/gtest.h"
#include "util/flags.h"
=======
#include "util/test.h"
>>>>>>> CHANGE (84daac Remove a condition from exhaustive1_test.cc that is no longe)
#include "re2/testing/exhaustive_tester.h"

namespace re2 {

// Test simple repetition operators
TEST(Repetition, Simple) {
  std::vector<std::string> ops = Split(" ",
    "%s{0} %s{0,} %s{1} %s{1,} %s{0,1} %s{0,2} "
    "%s{1,2} %s{2} %s{2,} %s{3,4} %s{4,5} "
    "%s* %s+ %s? %s*? %s+? %s??");
  ExhaustiveTest(3, 2, Explode("abc."), ops,
                 6, Explode("ab"), "(?:%s)", "");
  ExhaustiveTest(3, 2, Explode("abc."), ops,
                 40, Explode("a"), "(?:%s)", "");
}

// Test capturing parens -- (a) -- inside repetition operators
TEST(Repetition, Capturing) {
  std::vector<std::string> ops = Split(" ",
    "%s{0} %s{0,} %s{1} %s{1,} %s{0,1} %s{0,2} "
    "%s{1,2} %s{2} %s{2,} %s{3,4} %s{4,5} "
    "%s* %s+ %s? %s*? %s+? %s??");
  ExhaustiveTest(3, 2, Split(" ", "a (a) b"), ops,
                 7, Explode("ab"), "(?:%s)", "");
  ExhaustiveTest(3, 2, Split(" ", "a (a)"), ops,
                 50, Explode("a"), "(?:%s)", "");
}

}  // namespace re2
