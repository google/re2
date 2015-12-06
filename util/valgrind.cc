// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "util/util.h"
#ifndef _WIN32
#include "util/valgrind.h"
#endif

namespace re2 {

bool RunningOnValgrind() {
#ifdef RUNNING_ON_VALGRIND
  return RUNNING_ON_VALGRIND != 0;
#else
  return false;
#endif
}

}  // namespace re2
