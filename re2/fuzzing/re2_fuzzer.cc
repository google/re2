// Copyright 2016 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <map>
#include <string>

#include "re2/re2.h"

using re2::StringPiece;
using std::string;

// NOT static, NOT signed.
uint8_t dummy = 0;

void Test(StringPiece pattern, const RE2::Options& options, StringPiece text) {
  RE2 re(pattern, options);
  if (!re.ok())
    return;

  // Don't waste time fuzzing high-size programs.
  // (They can cause bug reports due to fuzzer timeouts.)
  int size = re.ProgramSize();
  if (size > 9999)
    return;

  // Don't waste time fuzzing high-fanout programs.
  // (They can also cause bug reports due to fuzzer timeouts.)
  std::map<int, int> histogram;
  int fanout = re.ProgramFanout(&histogram);
  if (fanout > 9)
    return;

  StringPiece sp1, sp2, sp3, sp4;
  string s1, s2, s3, s4;
  int i1, i2, i3, i4;
  double d1, d2, d3, d4;

  RE2::FullMatch(text, re, &sp1, &sp2, &sp3, &sp4);
  RE2::PartialMatch(text, re, &s1, &s2, &s3, &s4);

  sp1 = sp2 = text;
  RE2::Consume(&sp1, re, &i1, &i2, &i3, &i4);
  RE2::FindAndConsume(&sp2, re, &d1, &d2, &d3, &d4);

  s3 = s4 = text.ToString();
  RE2::Replace(&s3, re, "");
  RE2::GlobalReplace(&s4, re, "");

  // Exercise some other API functionality.
  dummy += re.NumberOfCapturingGroups();
  dummy += RE2::QuoteMeta(pattern).size();
}

// Entry point for libFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size == 0 || size > 1024)
    return 0;

  // Crudely limit the use of \p and \P.
  // Otherwise, we will waste time on inputs that have long runs of Unicode
  // character classes. The fuzzer has shown itself to be easily capable of
  // generating such patterns that fall within the other limits, but result
  // in timeouts nonetheless. The marginal cost is high - even more so when
  // counted repetition is involved - whereas the marginal benefit is zero.
  int backslash_p = 0;
  for (size_t i = 0; i < size; i++) {
    if (data[i] == '\\' && i+1 < size && (data[i+1] == 'p' || data[i+1] == 'P'))
      backslash_p++;
  }
  if (backslash_p > 10)
    return 0;

  // The one-at-a-time hash by Bob Jenkins.
  uint32_t hash = 0;
  for (size_t i = 0; i < size; i++) {
    hash += data[i];
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  RE2::Options options;
  options.set_log_errors(false);
  options.set_max_mem(64 << 20);
  options.set_encoding(hash & 1 ? RE2::Options::EncodingLatin1
                                : RE2::Options::EncodingUTF8);
  options.set_posix_syntax(hash & 2);
  options.set_longest_match(hash & 4);
  options.set_literal(hash & 8);
  options.set_never_nl(hash & 16);
  options.set_dot_nl(hash & 32);
  options.set_never_capture(hash & 64);
  options.set_case_sensitive(hash & 128);
  options.set_perl_classes(hash & 256);
  options.set_word_boundary(hash & 512);
  options.set_one_line(hash & 1024);

  const char* ptr = reinterpret_cast<const char*>(data);
  int len = static_cast<int>(size);

  StringPiece pattern(ptr, len);
  StringPiece text(ptr, len);
  Test(pattern, options, text);

  return 0;
}
