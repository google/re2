// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef RE2_UTIL_HASH_MAP_H__
#define RE2_UTIL_HASH_MAP_H__

#ifdef __GNUC__
#include <ext/hash_map>
#include <ext/hash_set>

namespace re2 {
  using namespace __gnu_cxx;
}  // namespace re2

#else

#include <hash_map>
#include <hash_set>

#endif

#endif  // RE2_UTIL_HASH_MAP_H__

