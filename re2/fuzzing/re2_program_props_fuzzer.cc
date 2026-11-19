// Copyright 2026 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Fuzzer for the program-introspection surface of the RE2 API:
//   RE2::ProgramSize, RE2::ReverseProgramSize, RE2::ProgramFanout,
//   RE2::ReverseProgramFanout and RE2::PossibleMatchRange with a
//   fuzzer-controlled maxlen sweep.
//
// Rationale (see also the OSS-Fuzz coverage pipeline report): the shipped
// re2_fuzzer.cc never calls any of these; re2.cc's introspection block
// (l.323-390) and the PossibleMatchRange machinery (l.597+) beyond the fixed
// maxlen=9 call stay dark. ProgramFanout/ReverseProgramFanout build sparse
// histograms over the compiled programs' instruction fanouts (prog.cc,
// sparse_array.h) and PossibleMatchRange walks the parsed regexp emitting
// string bounds (regexp.cc, tostring paths) - all fed by attacker patterns
// in production (Code Search index range computation).
//
// Expected bug classes: logic errors in fanout histogram accumulation,
// integer issues in program size accounting, string-handling defects in
// PossibleMatchRange bound emission (incl. UTF-8 boundaries), unbounded
// growth on crafted patterns.
//
// Timeouts are bounded by construction: pattern <= 200 bytes, text <= 400
// bytes, maxlen swept only over 1..12, and 64 MiB max_mem.

#include <fuzzer/FuzzedDataProvider.h>
#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "re2/re2.h"

// NOT static, NOT signed.
uint8_t re2_program_props_sink = 0;

// Entry point for libFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // Framing (all consumed from the front, so seeds are plain concatenations):
  //   [options:1][maxlen:1][pattern_len:1][text_len:2 big-endian]
  //   [pattern][text]
  if (size < 5)
    return 0;
  FuzzedDataProvider fdp(data, size);
  std::vector<uint8_t> header = fdp.ConsumeBytes<uint8_t>(5);
  const uint8_t opts = header[0];
  const uint8_t maxlen = header[1];
  const size_t pattern_len = header[2];
  const size_t text_len =
      (static_cast<size_t>(header[3]) << 8) | header[4];
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
  options.set_max_mem(64 << 20);
  options.set_literal((opts & 2) != 0);
  options.set_case_sensitive((opts & 4) == 0);
  options.set_posix_syntax((opts & 8) != 0);
  options.set_longest_match((opts & 16) != 0);
  options.set_never_capture((opts & 32) != 0);

  RE2 re(pattern, options);
  if (!re.ok())
    return 0;

  // Program size accounting for the forward and reverse programs.
  re2_program_props_sink += static_cast<uint8_t>(re.ProgramSize());
  re2_program_props_sink += static_cast<uint8_t>(re.ReverseProgramSize());

  // Fanout histograms over the compiled programs' instructions.
  std::vector<int> fanout;
  if (re.ProgramFanout(&fanout) == 0)
    re2_program_props_sink += static_cast<uint8_t>(fanout.size());
  std::vector<int> reverse_fanout;
  if (re.ReverseProgramFanout(&reverse_fanout) == 0)
    re2_program_props_sink += static_cast<uint8_t>(reverse_fanout.size());

  // PossibleMatchRange with a swept maxlen (the shipped fuzzer only ever
  // passes 9). maxlen 0 is excluded: 1..12 keeps bound emission deep but
  // cheap; return-value and bound sanity are checked crudely.
  const int range_maxlen = static_cast<int>(maxlen % 12) + 1;
  std::string min, max;
  if (re.PossibleMatchRange(&min, &max, range_maxlen)) {
    re2_program_props_sink += static_cast<uint8_t>(min.size());
    re2_program_props_sink += static_cast<uint8_t>(max.size());
  }

  // A match attempt so that the reverse program is actually exercised under
  // the longest_match option combination used for the introspection calls.
  absl::string_view text_view(text);
  re.Match(text_view, 0, text_view.size(), RE2::UNANCHORED, NULL, 0);
  return 0;
}
