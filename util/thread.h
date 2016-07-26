// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_THREAD_H_
#define UTIL_THREAD_H_

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

#endif  // UTIL_THREAD_H_
