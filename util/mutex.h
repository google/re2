// Copyright 2007 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_MUTEX_H_
#define UTIL_MUTEX_H_

#include <absl/synchronization/mutex.h>

namespace re2 {

using absl::Mutex;
using absl::MutexLock;
using absl::ReaderMutexLock;
using absl::WriterMutexLock;

// Catch bug where variable name is omitted, e.g. MutexLock (&mu);
#define MutexLock(x) static_assert(false, "MutexLock declaration missing variable name")
#define ReaderMutexLock(x) static_assert(false, "ReaderMutexLock declaration missing variable name")
#define WriterMutexLock(x) static_assert(false, "WriterMutexLock declaration missing variable name")

}  // namespace re2

#endif  // UTIL_MUTEX_H_
