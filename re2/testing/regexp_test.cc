// Copyright 2006 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Test parse.cc, dump.cc, and tostring.cc.

#include <string>
#include <vector>
#include "util/test.h"
#include "re2/regexp.h"

namespace re2 {

// Test that overflowed ref counts work.
TEST(Regexp, BigRef) {
  Regexp* re;
  
  re = Regexp::Parse("x", Regexp::NoParseFlags, NULL);
  for (int i = 0; i < 100000; i++)
    re->Incref();
  for (int i = 0; i < 100000; i++)
    re->Decref();
  CHECK_EQ(re->Ref(), 1);
  re->Decref();
}

// Test that very large Concats work.
// Depends on overflowed ref counts working.
TEST(Regexp, BigConcat) {
  Regexp* x;
  x = Regexp::Parse("x", Regexp::NoParseFlags, NULL);
  vector<Regexp*> v(90000, x);  // ToString bails out at 100000
  for (int i = 0; i < v.size(); i++)
    x->Incref();
  CHECK_EQ(x->Ref(), 1 + v.size()) << x->Ref();
  Regexp* re = Regexp::Concat(&v[0], v.size(), Regexp::NoParseFlags);
  CHECK_EQ(re->ToString(), string(v.size(), 'x'));
  re->Decref();
  CHECK_EQ(x->Ref(), 1) << x->Ref();
  x->Decref();
}

}  // namespace re2
