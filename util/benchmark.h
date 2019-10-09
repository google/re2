// Copyright 2009 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_BENCHMARK_H_
#define UTIL_BENCHMARK_H_

#include <stdint.h>
#include <functional>
#include <thread>

namespace testing {

class Benchmark {
 public:
  Benchmark(const char* name, void (*func)(int))
      : name_(name),
        func_([func](int iters, int arg) { func(iters); }),
        lo_(0),
        hi_(0),
        arg_(false) {
    Register();
  }

  Benchmark(const char* name, void (*func)(int, int), int lo, int hi)
      : name_(name),
        func_([func](int iters, int arg) { func(iters, arg); }),
        lo_(lo),
        hi_(hi),
        arg_(true) {
    Register();
  }

  Benchmark* ThreadRange(int lo, int hi) {
    // Pretend to support multi-threaded benchmarking.
    return this;
  }

  const char* name() const { return name_; }
  const std::function<void(int, int)>& func() const { return func_; }
  int lo() const { return lo_; }
  int hi() const { return hi_; }
  bool arg() const { return arg_; }

 private:
  void Register();

  const char* name_;
  std::function<void(int, int)> func_;
  int lo_;
  int hi_;
  bool arg_;

  Benchmark(const Benchmark&) = delete;
  Benchmark& operator=(const Benchmark&) = delete;
};

}  // namespace testing

void BenchmarkMemoryUsage();
void StartBenchmarkTiming();
void StopBenchmarkTiming();
void SetBenchmarkBytesProcessed(int64_t b);
void SetBenchmarkItemsProcessed(int i);

int NumCPUs();

#define BENCHMARK(f)                     \
  ::testing::Benchmark* _benchmark_##f = \
      (new ::testing::Benchmark(#f, f))

#define BENCHMARK_RANGE(f, lo, hi)       \
  ::testing::Benchmark* _benchmark_##f = \
      (new ::testing::Benchmark(#f, f, lo, hi))

#endif  // UTIL_BENCHMARK_H_
