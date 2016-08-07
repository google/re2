// Copyright 2005-2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Modified from Google perftools's tcmalloc_unittest.cc.

#include <stdint.h>

#include "util/random.h"

namespace re2 {

int32_t ACMRandom::Next() {
  const int32_t M = 2147483647L;   // 2^31-1
  const int32_t A = 16807;
  // In effect, we are computing seed_ = (seed_ * A) % M, where M = 2^31-1
  uint32_t lo = A * (int32_t)(seed_ & 0xFFFF);
  uint32_t hi = A * (int32_t)((uint32_t)seed_ >> 16);
  lo += (hi & 0x7FFF) << 16;
  if (lo > M) {
    lo &= M;
    ++lo;
  }
  lo += hi >> 15;
  if (lo > M) {
    lo &= M;
    ++lo;
  }
  return (seed_ = (int32_t)lo);
}

int32_t ACMRandom::Uniform(int32_t n) {
  return Next() % n;
}

}  // namespace re2
