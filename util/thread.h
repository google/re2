// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef RE2_UTIL_THREAD_H__
#define RE2_UTIL_THREAD_H__

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

class Thread {
 public:
  Thread();
  virtual ~Thread();
  void Start();
  void Join();
  void SetJoinable(bool);
  virtual void Run() = 0;

 private:
#ifdef _WIN32
  HANDLE pid_;
#else
  pthread_t pid_;
#endif
  bool running_;
  bool joinable_;
};

#endif  // RE2_UTIL_THREAD_H__
