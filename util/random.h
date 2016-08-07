// Copyright 2005-2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_RANDOM_H_
#define UTIL_RANDOM_H_

// Modified from Google perftools's tcmalloc_unittest.cc.

#include <stdint.h>

#include "util/util.h"

namespace re2 {

// ACM minimal standard random number generator.  (re-entrant.)
class ACMRandom {
 public:
  ACMRandom(int32_t seed) : seed_(seed) {}
  int32_t Next();
  int32_t Uniform(int32_t);

  void Reset(int32_t seed) { seed_ = seed; }

 private:
  int32_t seed_;
};

}  // namespace re2

#endif  // UTIL_RANDOM_H_
