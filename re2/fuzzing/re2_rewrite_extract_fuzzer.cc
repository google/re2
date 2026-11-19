// Copyright 2026 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Fuzzer for the rewrite/extraction surface of the RE2 API:
//   RE2::CheckRewriteString, RE2::MaxSubmatch, RE2::Rewrite,
//   RE2::Extract, RE2::Replace, RE2::GlobalReplace and the array-based
//   typed extraction interface (FullMatchN/PartialMatchN/ConsumeN/
//   FindAndConsumeN), including the string-typed Arg conversions.
//
// Rationale (see also the OSS-Fuzz coverage pipeline report): the shipped
// re2_fuzzer.cc never passes a non-empty rewrite to Replace/GlobalReplace and
// never calls Extract/CheckRewriteString/Rewrite/MaxSubmatch or the *MatchN
// family, so the rewrite token machinery (\0-\9, \\, invalid escapes),
// the Extract submatch bookkeeping and the string/char-typed Arg parsers are
// dark in production fuzzing. Attacker-controlled (pattern, text, rewrite)
// triples are the trust boundary of every search&replace UI, log scrubber and
// templating layer that uses these APIs.
//
// Expected bug classes: out-of-bounds string writes or iterators invalidated
// by string reallocation inside Replace/GlobalReplace/Rewrite; logic errors
// in submatch offset handling; UBSan findings in the numeric Arg parsers;
// memory leaks on extraction paths.
//
// Timeouts are bounded by construction: inputs are length-framed and small
// (pattern <= 200, rewrite <= 200, text <= 400 bytes), GlobalReplace is
// bounded via the upstream-provided FUZZING_ONLY knob (16 iterations), and
// matching itself is linear-time under the 64 MiB max_mem budget.

#include <fuzzer/FuzzedDataProvider.h>
#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "re2/re2.h"

// NOT static, NOT signed.
uint8_t re2_rewrite_extract_sink = 0;

// Entry point for libFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // All framing is consumed from the front of the input so that seed corpus
  // files are plain concatenations:
  //   [options:1][pattern_len:1][rewrite_len:1][text_len:2 big-endian]
  //   [pattern][rewrite][text]
  if (size < 5)
    return 0;
  FuzzedDataProvider fdp(data, size);
  std::vector<uint8_t> header = fdp.ConsumeBytes<uint8_t>(5);
  const uint8_t opts = header[0];
  const size_t pattern_len = header[1];
  const size_t rewrite_len = header[2];
  const size_t text_len =
      (static_cast<size_t>(header[3]) << 8) | header[4];
  if (pattern_len > 200 || rewrite_len > 200 || text_len > 400)
    return 0;

  std::string pattern, rewrite, text;
  for (int which = 0; which < 3; which++) {
    const size_t len = (which == 0) ? pattern_len
                     : (which == 1) ? rewrite_len
                                    : text_len;
    std::vector<uint8_t> bytes = fdp.ConsumeBytes<uint8_t>(len);
    bytes.resize(len, ' ');  // keep lengths exact if the input ran short
    if (which == 0)
      pattern.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    else if (which == 1)
      rewrite.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    else
      text.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  }
  if (pattern.empty())
    return 0;

  RE2::Options options;
  options.set_encoding((opts & 1) ? RE2::Options::EncodingLatin1
                                  : RE2::Options::EncodingUTF8);
  options.set_log_errors(false);
  options.set_max_mem(64 << 20);
  options.set_literal((opts & 2) != 0);
  options.set_never_nl((opts & 4) != 0);
  options.set_case_sensitive((opts & 8) == 0);
  options.set_posix_syntax((opts & 16) != 0);
  options.set_longest_match((opts & 32) != 0);

  // Upstream knob (re2.h) that bounds GlobalReplace() iterations; it exists
  // exactly for fuzzers like this one.
  RE2::FUZZING_ONLY_set_maximum_global_replace_count(16);

  RE2 re(pattern, options);
  if (!re.ok())
    return 0;

  // Rewrite-surface primitives: validation, max-submatch, direct rewrites.
  std::string error;
  if (re.CheckRewriteString(rewrite, &error))
    re2_rewrite_extract_sink += static_cast<uint8_t>(error.size());
  const int maxsub = RE2::MaxSubmatch(rewrite);
  if (maxsub >= 0 && maxsub < 16) {
    // A fabricated, all-empty vector exercises the rewrite token loop alone.
    std::vector<absl::string_view> empty_vec(static_cast<size_t>(maxsub) + 1);
    std::string out;
    re.Rewrite(&out, rewrite, empty_vec.data(),
               static_cast<int>(empty_vec.size()));
    re2_rewrite_extract_sink += static_cast<uint8_t>(out.size());

    // A real match vector (when the pattern matches) exercises the snippet
    // append path with genuine submatch slices.
    absl::string_view vec[17];
    const int nvec = maxsub + 1;
    if (re.Match(text, 0, text.size(), RE2::UNANCHORED, vec, nvec)) {
      std::string out2;
      re.Rewrite(&out2, rewrite, vec, nvec);
      re2_rewrite_extract_sink += static_cast<uint8_t>(out2.size());
    }
  }

  // The composed rewrite APIs with attacker-controlled rewrites.
  std::string extracted;
  RE2::Extract(text, re, rewrite, &extracted);
  re2_rewrite_extract_sink += static_cast<uint8_t>(extracted.size());

  std::string replaced = text;
  RE2::Replace(&replaced, re, rewrite);
  re2_rewrite_extract_sink += static_cast<uint8_t>(replaced.size());

  std::string global = text;
  re2_rewrite_extract_sink +=
      static_cast<uint8_t>(RE2::GlobalReplace(&global, re, rewrite));

  // The array-based typed extraction interface, including the string-typed
  // Arg conversions (std::string, absl::string_view, char) that the shipped
  // fuzzer never exercises (it only extracts numeric types).
  int i;
  unsigned int u;
  long l;
  float f;
  double d;
  char c;
  std::string str;
  absl::string_view sv;
  const RE2::Arg args[] = {RE2::Arg(&i),   RE2::Arg(&u),  RE2::Arg(&l),
                           RE2::Arg(&f),   RE2::Arg(&d),  RE2::Arg(&c),
                           RE2::Arg(&str), RE2::Arg(&sv)};
  const RE2::Arg* const arg_ptrs[] = {&args[0], &args[1], &args[2], &args[3],
                                      &args[4], &args[5], &args[6], &args[7]};
  RE2::FullMatchN(text, re, arg_ptrs, 8);
  RE2::PartialMatchN(text, re, arg_ptrs, 8);
  absl::string_view consume = text;
  RE2::ConsumeN(&consume, re, arg_ptrs, 8);
  RE2::FindAndConsumeN(&consume, re, arg_ptrs, 8);
  return 0;
}
