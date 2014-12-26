// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <windows.h>

#include "util/util.h"
#include "util/thread.h"

Thread::Thread() {
  pid_ = 0;
  running_ = 0;
  joinable_ = 0;
}

Thread::~Thread() {
}

DWORD WINAPI startThread(void *v) {
  Thread* t = (Thread*)v;
  t->Run();
  return 0;
}

void Thread::Start() {
  CHECK(!running_);
  pid_ = CreateThread(NULL, 0, startThread, this, 0, NULL);
  running_ = true;
  if (!joinable_)
  {
    CloseHandle(pid_);
    pid_ = 0;
  }
}

void Thread::Join() {
  CHECK(running_);
  CHECK(joinable_);
  if (0 != pid_)
  {
      WaitForSingleObject(pid_, INFINITE);
  }
  running_ = 0;
}

void Thread::SetJoinable(bool j) {
  CHECK(!running_);
  joinable_ = j;
}
