// Copyright 2016 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_BITMAP_H_
#define UTIL_BITMAP_H_

#ifdef _MSC_VER
#include <intrin.h>
#endif
#include "util/util.h"

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

    return (words_[c / 64] & (1ULL << (c % 64))) != 0;
  }

  // Sets the bit with index c.
  void Set(int c) {
    DCHECK_GE(c, 0);
    DCHECK_LE(c, 255);

    words_[c / 64] |= (1ULL << (c % 64));
  }

  // Finds the next non-zero bit with index >= c.
  // Returns -1 if no such bit exists.
  int FindNextSetBit(int c) const;

 private:
  // Finds the least significant non-zero bit in n.
  static int FindLSBSet(uint64 n) {
    DCHECK_NE(n, 0);

#if defined(__GNUC__)
    return __builtin_ctzll(n);
#elif defined(_MSC_VER)
    unsigned long c;
    _BitScanForward64(&c, n);
    return static_cast<int>(c);
#else
#error "bit scan forward not implemented"
#endif
  }

  uint64 words_[4];
};

int Bitmap256::FindNextSetBit(int c) const {
  DCHECK_GE(c, 0);
  DCHECK_LE(c, 255);

  // Check the word that contains the bit. Mask out any lower bits.
  int i = c / 64;
  uint64 word = words_[i] & (~0ULL << (c % 64));
  if (word != 0)
    return (i * 64) + FindLSBSet(word);

  // Check any following words.
  i++;
  switch (i) {
    case 1:
      if (words_[1] != 0)
        return (1 * 64) + FindLSBSet(words_[1]);
      FALLTHROUGH_INTENDED;
    case 2:
      if (words_[2] != 0)
        return (2 * 64) + FindLSBSet(words_[2]);
      FALLTHROUGH_INTENDED;
    case 3:
      if (words_[3] != 0)
        return (3 * 64) + FindLSBSet(words_[3]);
      FALLTHROUGH_INTENDED;
    default:
      return -1;
  }
}

}  // namespace re2

#endif  // UTIL_BITMAP_H_
