// Copyright 2006-2008 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef RE2_UTIL_ATOMICOPS_H__
#define RE2_UTIL_ATOMICOPS_H__

#if defined(__i386__) 

static inline void MemoryBarrier() {
  int x;
  __asm__ __volatile__("xchgl (%0),%0"  // The lock prefix is implicit for xchg.
                       :: "r" (&x));
}

#elif defined(__x86_64__) 

// 64-bit implementations of memory barrier can be simpler, because it
// "mfence" is guaranteed to exist.
static inline void MemoryBarrier() {
  __asm__ __volatile__("mfence" : : : "memory");
}

#elif defined(__ppc__)

static inline void MemoryBarrier() {
  __asm__ __volatile__("eieio" : : : "memory");
}

#else

#include "util/mutex.h"

static inline void MemoryBarrier() {
  // Slight overkill, but good enough:
  // any mutex implementation must have
  // a read barrier after the lock operation and
  // a write barrier before the unlock operation.
  //
  // It may be worthwhile to write architecture-specific
  // barriers for the common platforms, as above, but
  // this is a correct fallback.
  re2::Mutex mu;
  re2::MutexLock l(&mu);
}

/*
#error Need MemoryBarrier for architecture.

// Windows
inline void MemoryBarrier() {
  LONG x;
  ::InterlockedExchange(&x, 0);
}
*/

#endif

#endif  // RE2_UTIL_ATOMICOPS_H__
