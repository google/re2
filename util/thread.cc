// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifdef WIN32
#include <windows.h>
typedef void* pthread_t;
#else
#include <pthread.h>
#endif

#include "util/util.h"
#include "util/thread.h"

Thread::Thread() {
  pid_ = 0;
  running_ = 0;
  joinable_ = 0;
}

Thread::~Thread() {
}

void *startThread(void *v) {
  Thread* t = (Thread*)v;
  t->Run();
  return 0;
}

void Thread::Start() {
  CHECK(!running_);
#ifdef WIN32
  pid_ = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)startThread, this, 0, NULL);
#else
  pthread_create(&pid_, 0, startThread, this);
#endif
  running_ = true;
  if (!joinable_)
#ifdef WIN32
    CloseHandle(pid_);
#else
    pthread_detach(pid_);
#endif
}

void Thread::Join() {
  CHECK(running_);
  CHECK(joinable_);
  void *val;
#ifdef WIN32
  val = (void*)WaitForSingleObject(pid_, INFINITE);
  CloseHandle(pid_);
#else
  pthread_join(pid_, &val);
#endif
  running_ = 0;
}

void Thread::SetJoinable(bool j) {
  CHECK(!running_);
  joinable_ = j;
}
