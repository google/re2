// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "util/util.h"
#include "util/valgrind.h"

namespace re2 {

static bool checkValgrind() {
#ifdef RUNNING_ON_VALGRIND
	return RUNNING_ON_VALGRIND;
#else
	return false;
#endif
}

static const int valgrind = checkValgrind();

int RunningOnValgrind() {
  return valgrind;
}

}  // namespace re2
