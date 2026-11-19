// Copyright 2026 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Fuzzer that biases RE2's matching ENGINE INTERIORS:
//   DFA cache behaviour and the DFA->NFA memory-budget fallback
//   (dfa.cc), the longest-match DFA specialization, the subtext
//   search loop in RE2::Match (re2.cc), onepass execution
//   (onepass.cc), NFA simulation with capture restore (nfa.cc) and
//   bitstate backtracking (bitstate.cc), plus prog.cc's byte-map,
//   prefix-acceleration and flatten machinery they depend on.
//
// Biasing knobs driven per input (the shipped re2_fuzzer fixes most
// of these):
//   * fuzzer-controlled max_mem down to 16 KiB (re2_fuzzer hardcodes
//     64 MiB) - small DFA budgets force cache flushes and the
//     documented fallback to the NFA implementation;
//   * 1..8 match rounds over shifted suffix/shrunk-end subtexts of
//     the same compiled program (cache churn, subtext boundary
//     handling in RE2::Match);
//   * assertion-heavy patterns (\A \z \b \B ^ $ via seeds), foldcase
//     byte ranges, POSIX/longest-match toggles (reverse program and
//     kLongestMatch DFA), dot_nl/never_nl/one_line flags;
//   * all three RE2::Anchor values and submatch extraction up to 9
//     slots (capture bookkeeping in every engine).
//
// Expected bug classes: memory-safety defects in the DFA cache and
// queue management, capture-index handling in NFA/bitstate/onepass,
// boundary arithmetic in the subtext loop, UB in byte-map/foldcase
// tables.
//
// Timeouts are bounded by construction: pattern <= 200, text <= 400
// bytes, <= 8 rounds of <= 2 linear-time matches each, and RE2's
// matching is linear-time under every budget.

#include <fuzzer/FuzzedDataProvider.h>
#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "re2/re2.h"

// NOT static, NOT signed.
uint8_t re2_engine_bias_sink = 0;

// Entry point for libFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Framing (all consumed from the front; seeds are plain concatenations):
  //   [options:1][maxmem_exp:1][rounds:1][pattern_len:1][text_len:2 BE]
  //   [pattern][text]
  if (size < 6)
    return 0;
  FuzzedDataProvider fdp(data, size);
  std::vector<uint8_t> header = fdp.ConsumeBytes<uint8_t>(6);
  const uint8_t opts = header[0];
  const uint8_t maxmem_exp = header[1];
  const uint8_t rounds_in = header[2];
  const size_t pattern_len = header[3];
  const size_t text_len =
      (static_cast<size_t>(header[4]) << 8) | header[5];
  if (pattern_len > 200 || text_len > 400)
    return 0;
  std::vector<uint8_t> pbytes = fdp.ConsumeBytes<uint8_t>(pattern_len);
  pbytes.resize(pattern_len, ' ');
  std::string pattern(reinterpret_cast<const char*>(pbytes.data()),
                      pbytes.size());
  std::vector<uint8_t> tbytes = fdp.ConsumeBytes<uint8_t>(text_len);
  tbytes.resize(text_len, ' ');
  std::string text(reinterpret_cast<const char*>(tbytes.data()),
                   tbytes.size());
  if (pattern.empty())
    return 0;

  RE2::Options options;
  options.set_encoding((opts & 1) ? RE2::Options::EncodingLatin1
                                  : RE2::Options::EncodingUTF8);
  options.set_log_errors(false);
  // The engine-bias knob: shrink the memory budget from 64 MiB down to
  // 16 KiB to force DFA cache flushes and the NFA fallback.
  const int shift = (maxmem_exp & 15) < 12 ? (maxmem_exp & 15) : 12;
  options.set_max_mem((64 << 20) >> shift);
  options.set_case_sensitive((opts & 2) == 0);
  options.set_posix_syntax((opts & 4) != 0);
  options.set_longest_match((opts & 8) != 0);
  options.set_dot_nl((opts & 16) != 0);
  options.set_never_nl((opts & 32) != 0);
  options.set_one_line((opts & 64) != 0);
  RE2::Anchor anchor;
  switch (opts >> 6) {
    case 1:  anchor = RE2::ANCHOR_START; break;
    case 2:  anchor = RE2::ANCHOR_BOTH;  break;
    default: anchor = RE2::UNANCHORED;  break;
  }

  RE2 re(pattern, options);
  if (!re.ok())
    return 0;

  const int ngroups = re.NumberOfCapturingGroups();
  const int ncap = ngroups < 8 ? 1 + ngroups : 9;
  const int rounds = 1 + (rounds_in & 7);

  // Match rounds over shifted starts (suffix subtexts) and shrunk ends:
  // exercises the subtext loop, boundary adjustment and both forward and
  // reverse program paths across the chosen budget and anchor.
  for (int r = 0; r < rounds; r++) {
    size_t start = (static_cast<size_t>(r) * (text.size() / 4 + 1));
    if (start > text.size())
      start = text.size();
    absl::string_view vec[9];
    re.Match(text, start, text.size(), anchor, vec, ncap);
    size_t end = text.size() - (r % 3);
    if (end > text.size() || end < start)
      end = start;
    re.Match(text, 0, end, anchor, vec, ncap);
    // Unanchored whole-text match with reduced submatch slots, so capture
    // bookkeeping is exercised at several slot counts against one program.
    re.Match(text, 0, text.size(), anchor, vec, ncap > 1 ? ncap - 1 : 0);
  }
  re2_engine_bias_sink += static_cast<uint8_t>(re.ProgramSize());
  return 0;
}
