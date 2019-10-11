// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_FLAGS_H_
#define UTIL_FLAGS_H_

// Simplified version of Google's command line flags.
// Does not support parsing the command line.
// If you want to do that, see
// https://gflags.github.io/gflags/

#define DEFINE_FLAG(type, name, deflt, desc) \
	namespace re2 { type FLAGS_##name = deflt; }

#define DECLARE_FLAG(type, name) \
	namespace re2 { extern type FLAGS_##name; }

#endif  // UTIL_FLAGS_H_
