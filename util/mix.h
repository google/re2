// Copyright 2016 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_MIX_H_
#define UTIL_MIX_H_

#include <stddef.h>
#include <limits>

namespace re2 {

class HashMix {
 public:
  HashMix() : hash_(1) {}
  explicit HashMix(size_t val) : hash_(val + 83) {}
  void Mix(size_t val) {
    static const size_t kMul = static_cast<size_t>(0xdc3eb94af8ab4c93ULL);
    hash_ *= kMul;
    hash_ = ((hash_ << 19) |
             (hash_ >> (std::numeric_limits<size_t>::digits - 19))) + val;
  }
  size_t get() const { return hash_; }
 private:
  size_t hash_;
};

}  // namespace re2

#endif  // UTIL_MIX_H_
