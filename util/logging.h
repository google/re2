// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Simplified version of Google's logging.

#ifndef RE2_UTIL_LOGGING_H__
#define RE2_UTIL_LOGGING_H__

#include <stdio.h>  /* for fwrite */
#include <sstream>

#include "util/util.h"
#include "util/flags.h"

DECLARE_int32(minloglevel);

// Debug-only checking.
#define DCHECK(condition) assert(condition)
#define DCHECK_EQ(val1, val2) assert((val1) == (val2))
#define DCHECK_NE(val1, val2) assert((val1) != (val2))
#define DCHECK_LE(val1, val2) assert((val1) <= (val2))
#define DCHECK_LT(val1, val2) assert((val1) < (val2))
#define DCHECK_GE(val1, val2) assert((val1) >= (val2))
#define DCHECK_GT(val1, val2) assert((val1) > (val2))

// Always-on checking
#define CHECK(x)	if(x){}else LogMessageFatal(__FILE__, __LINE__).stream() << "Check failed: " #x
#define CHECK_LT(x, y)	CHECK((x) < (y))
#define CHECK_GT(x, y)	CHECK((x) > (y))
#define CHECK_LE(x, y)	CHECK((x) <= (y))
#define CHECK_GE(x, y)	CHECK((x) >= (y))
#define CHECK_EQ(x, y)	CHECK((x) == (y))
#define CHECK_NE(x, y)	CHECK((x) != (y))

#define LOG_INFO LogMessage(__FILE__, __LINE__, 0)
#define LOG_WARNING LogMessage(__FILE__, __LINE__, 1)
#define LOG_ERROR LogMessage(__FILE__, __LINE__, 2)
#define LOG_FATAL LogMessageFatal(__FILE__, __LINE__)
#define LOG_QFATAL LOG_FATAL

// It seems that one of the Windows header files defines ERROR as 0.
#ifdef _WIN32
#define LOG_0 LOG_INFO
#endif

#ifdef NDEBUG
#define DEBUG_MODE 0
#define LOG_DFATAL LOG_ERROR
#else
#define DEBUG_MODE 1
#define LOG_DFATAL LOG_FATAL
#endif

#define LOG(severity) LOG_ ## severity.stream()

#define VLOG(x) if((x)>0){}else LOG_INFO.stream()

class LogMessage {
 public:
  LogMessage(const char* file, int line, int severity)
      : severity_(severity), flushed_(false) {
    stream() << file << ":" << line << ": ";
  }
  void Flush() {
    stream() << "\n";
    if (severity_ >= re2::FLAGS_minloglevel) {
      string s = str_.str();
      size_t n = s.size();
      if (fwrite(s.data(), 1, n, stderr) < n) {}  // shut up gcc
    }
    flushed_ = true;
  }
  ~LogMessage() {
    if (!flushed_) {
      Flush();
    }
  }
  ostream& stream() { return str_; }

 private:
  const int severity_;
  bool flushed_;
  std::ostringstream str_;
  DISALLOW_COPY_AND_ASSIGN(LogMessage);
};

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4722) // destructor never returns
#endif

class LogMessageFatal : public LogMessage {
 public:
  LogMessageFatal(const char* file, int line)
      : LogMessage(file, line, 3) {}
  ~LogMessageFatal() {
    Flush();
    abort();
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(LogMessageFatal);
};

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif  // RE2_UTIL_LOGGING_H__
