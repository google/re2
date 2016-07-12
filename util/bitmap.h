// Copyright 2016 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef RE2_UTIL_BITMAP_H__
#define RE2_UTIL_BITMAP_H__

#include "util/util.h"
#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace re2 {

class Bitmap256 {
 public:
  Bitmap256() {
    memset(words_, 0, sizeof words_);
  }

  // Tests the bit with index c.
  bool Test(int c) const {
    DCHECK_GE(c, 0);
    DCHECK_LE(c, 255);

    return (words_[c >> 6] & (1ULL << (c & 63))) != 0;
  }

  // Sets the bit with index c.
  void Set(int c) {
    DCHECK_GE(c, 0);
    DCHECK_LE(c, 255);

    words_[c >> 6] |= (1ULL << (c & 63));
  }

  // Finds the next non-zero bit with index >= c.
  // Returns -1 if no such bit exists.
  int FindNextSetBit(int c) const {
    DCHECK_GE(c, 0);
    DCHECK_LE(c, 255);

    // Mask out any lower bits.
    int i = c >> 6;
    uint64 word = words_[i] & (~0ULL << (c & 63));
    if (word != 0)
      return (i << 6) + FindLSBSet(word);
    i++;
    switch (i) {
      case 1:
        if (words_[1] != 0)
          return (1 << 6) + FindLSBSet(words_[1]);
        // Fall through.
      case 2:
        if (words_[2] != 0)
          return (2 << 6) + FindLSBSet(words_[2]);
        // Fall through.
      case 3:
        if (words_[3] != 0)
          return (3 << 6) + FindLSBSet(words_[3]);
        // Fall through.
      default:
        return -1;
    }
  }

  // Finds the previous non-zero bit with index <= c.
  // Returns -1 if no such bit exists.
  int FindPrevSetBit(int c) const {
    DCHECK_GE(c, 0);
    DCHECK_LE(c, 255);

    int i = c >> 6;
    // Mask out any higher bits.
    uint64 word = words_[i] & ~((~0ULL - 1) << (c & 63));
    if (word != 0)
      return (i << 6) + FindMSBSet(word);
    i--;
    switch (i) {
      case 2:
        if (words_[2] != 0)
          return (2 << 6) + FindMSBSet(words_[2]);
        // Fall through.
      case 1:
        if (words_[1] != 0)
          return (1 << 6) + FindMSBSet(words_[1]);
        // Fall through.
      case 0:
        if (words_[0] != 0)
          return (0 << 6) + FindMSBSet(words_[0]);
        // Fall through.
      default:
        return -1;
    }
  }

 private:
  // Finds the least significant non-zero bit in n.
  static int FindLSBSet(uint64 n) {
    DCHECK_NE(n, 0);

#if defined(__GNUC__)
    return __builtin_ctzll(n);
#elif defined(_MSC_VER)
    int c;
    _BitScanForward64(&c, n);
    return c;
#else
#error "bit scan forward not implemented"
#endif
  }

  // Finds the most significant non-zero bit in n.
  static int FindMSBSet(uint64 n) {
    DCHECK_NE(n, 0);

#if defined(__GNUC__)
    return 63 - __builtin_clzll(n);
#elif defined(_MSC_VER)
    int c;
    _BitScanReverse64(&c, n);
    return c;
#else
#error "bit scan reverse not implemented"
#endif
  }

  uint64 words_[4];
};

}  // namespace re2

#endif  // RE2_UTIL_BITMAP_H__
