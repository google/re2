// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_

// TODO(junyer): Get rid of this.
#include <string>
using std::string;

#ifdef _WIN32

#define snprintf _snprintf_s
#define stricmp _stricmp
#define strtof strtod /* not really correct but best we can do */
#define strtoll _strtoi64
#define strtoull _strtoui64
#define vsnprintf vsnprintf_s

#pragma warning(disable: 4200) // zero-sized array

#endif

namespace re2 {

// DISALLOW_COPY_AND_ASSIGN disallows the copy and operator= functions.
// It goes in the private: declarations in a class.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

#define arraysize(array) (int)(sizeof(array)/sizeof((array)[0]))

#ifndef FALLTHROUGH_INTENDED
#define FALLTHROUGH_INTENDED do { } while (0)
#endif

#ifndef NO_THREAD_SAFETY_ANALYSIS
#define NO_THREAD_SAFETY_ANALYSIS
#endif

class StringPiece;

string CEscape(const StringPiece& src);
int CEscapeString(const char* src, int src_len, char* dest, int dest_len);

extern string StringPrintf(const char* format, ...);
extern void SStringPrintf(string* dst, const char* format, ...);
extern void StringAppendF(string* dst, const char* format, ...);
extern string PrefixSuccessor(const StringPiece& prefix);

}  // namespace re2

#include "util/logging.h"
#include "util/mutex.h"
#include "util/utf.h"

#endif  // UTIL_UTIL_H_
