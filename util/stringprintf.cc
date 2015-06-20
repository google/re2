// Copyright 2002 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "util/util.h"

namespace re2 {

#ifdef WIN32
static int CalculateBufferLen(const char* format, va_list args)
{
    return (_vscprintf(format, args) + 1);  // +1 for NULL char.
}
#else
static int CheckLength(const char* format, va_list ap, int length)
{
  char* space = new char[length];

  // It's possible for methods that use a va_list to invalidate
  // the data in it upon use.  The fix is to make a copy
  // of the structure before using it and use that copy instead.
  va_list backup_ap;
  va_copy(backup_ap, ap);
  int result = vsnprintf(space, sizeof(space), format, backup_ap);
  va_end(backup_ap);

  delete [] space;

  return result;
}

static int CalculateBufferLen(const char* format, va_list ap)
{
  // First try with a small fixed size buffer
  int length = 1024;
  int result = -1;

  while ((result = CheckLength(format, ap, length)) < 0)
  {
    // While our buffer is not big enough, just double the size.
    length *= 2;
  }

  // We need exactly "result+1" characters
  return (result + 1);
}
#endif

static void StringAppendV(string* dst, const char* format, va_list ap) {
  int length = CalculateBufferLen(format, ap);

  // This can be 1 char longer than needed,
  char* buf = new char[length + 1];

  int result = vsnprintf_s(buf, length, length - 1, format, ap);

  if ((result >= 0) && (result < length)) {
    // It fit
    dst->append(buf, result);
    delete[] buf;
    return;
  }
  delete[] buf;
}

string StringPrintf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  string result;
  StringAppendV(&result, format, ap);
  va_end(ap);
  return result;
}

void SStringPrintf(string* dst, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  dst->clear();
  StringAppendV(dst, format, ap);
  va_end(ap);
}

void StringAppendF(string* dst, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringAppendV(dst, format, ap);
  va_end(ap);
}

}  // namespace re2
