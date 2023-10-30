// Copyright 2006-2023 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Benchmark measuring a performance gain from the DFA loop optimization

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <utility>

#include "gtest/gtest.h"
#include "benchmark/benchmark.h"
#include "re2/re2.h"

namespace re2 {

void LongMatchStateString(benchmark::State& state) {
  state.PauseTiming();

  RE2::Options options;
  options.set_longest_match(true);
  RE2 re("(x+)", options);

  std::string text;
  text.append(4, ' ');
  text.append((size_t)state.range(0), 'x');
  text.append(4, ' ');

  absl::string_view m;

  state.ResumeTiming();
  for (auto _ : state)
    RE2::PartialMatch(text, re, &m);
}

enum {
  KB = 1024,
  MB = 1024 * 1024,
};

BENCHMARK(LongMatchStateString)->Iterations(16)->Arg(512 * KB)->Arg(4 * MB)->Arg(32 * MB);

}  // namespace re2
