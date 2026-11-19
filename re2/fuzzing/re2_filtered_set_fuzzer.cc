// Copyright 2026 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Fuzzer for the multi-pattern surfaces of the RE2 API:
//   RE2::Set (Add with error reporting, Compile, all three Match overloads
//   including the bool-only and ErrorInfo forms, move construction and move
//   assignment) and re2::FilteredRE2 (Add, Compile, SlowFirstMatch,
//   FirstMatch, AllMatches, AllPotentials) driven with REALISTIC atom
//   feedback: the atoms that actually match the text (computed via
//   RE2::PartialMatch), not just "all atoms matched".
//
// Rationale (see also the OSS-Fuzz coverage pipeline report): the shipped
// re2_fuzzer.cc drives Set and FilteredRE2 with a single pattern and always
// pretends that every atom matched, leaving PrefilterTree::PropagateMatch
// pruning, the partial-feedback paths, SlowFirstMatch/FirstMatch/
// AllPotentials and set.cc's error/move paths dark (prefilter.cc 89 +
// prefilter_tree.cc 65 + filtered_re2.cc 53 + set.cc 59 = 266 dark lines).
// FilteredRE2 is the production multi-pattern filter (search engines use it
// to shortlist regexps by required literals), so attacker-controlled pattern
// sets and text are the trust boundary.
//
// Expected bug classes: logic errors in atom-index bookkeeping and tree
// pruning; out-of-bounds vector indexing in prefilter propagation; memory
// leaks in the Prefilter node graph; use-after-move on RE2::Set.
//
// Timeouts are bounded by construction: at most 8 patterns of 100 bytes,
// text of 400 bytes, atom processing capped at 64, and 64 MiB max_mem.

#include <fuzzer/FuzzedDataProvider.h>
#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "absl/strings/string_view.h"
#include "re2/filtered_re2.h"
#include "re2/re2.h"
#include "re2/set.h"

// NOT static, NOT signed.
uint8_t re2_filtered_set_sink = 0;

// Entry point for libFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // All framing is consumed from the front of the input so that seed corpus
  // files are plain concatenations:
  //   [options:1][npatterns:1][text_len:2 big-endian]
  //   [plen_0..plen_{n-1}][pattern_0..pattern_{n-1}][text]
  if (size < 4)
    return 0;
  FuzzedDataProvider fdp(data, size);
  std::vector<uint8_t> header = fdp.ConsumeBytes<uint8_t>(4);
  const uint8_t opts = header[0];
  const uint8_t npatterns = header[1];
  const size_t text_len =
      (static_cast<size_t>(header[2]) << 8) | header[3];
  if (npatterns == 0 || npatterns > 8 || text_len > 400)
    return 0;

  std::vector<uint8_t> plens = fdp.ConsumeBytes<uint8_t>(npatterns);
  plens.resize(npatterns, 1);
  std::vector<std::string> patterns(npatterns);
  for (uint8_t pi = 0; pi < npatterns; pi++) {
    if (plens[pi] > 100)
      return 0;
    std::vector<uint8_t> bytes = fdp.ConsumeBytes<uint8_t>(plens[pi]);
    bytes.resize(plens[pi], ' ');
    patterns[pi].assign(reinterpret_cast<const char*>(bytes.data()),
                        bytes.size());
  }
  std::vector<uint8_t> tbytes = fdp.ConsumeBytes<uint8_t>(text_len);
  tbytes.resize(text_len, ' ');
  std::string text(reinterpret_cast<const char*>(tbytes.data()),
                   tbytes.size());

  RE2::Options options;
  options.set_encoding((opts & 1) ? RE2::Options::EncodingLatin1
                                  : RE2::Options::EncodingUTF8);
  options.set_log_errors(false);
  options.set_max_mem(64 << 20);
  options.set_case_sensitive((opts & 2) == 0);
  options.set_posix_syntax((opts & 4) != 0);
  options.set_longest_match((opts & 8) != 0);
  options.set_one_line((opts & 16) != 0);
  options.set_word_boundary((opts & 32) != 0);
  RE2::Anchor anchor;
  switch ((opts >> 6) & 3) {
    case 1:  anchor = RE2::ANCHOR_START; break;
    case 2:  anchor = RE2::ANCHOR_BOTH;  break;
    default: anchor = RE2::UNANCHORED;  break;
  }

  // Part 1: RE2::Set with error reporting, all Match overloads and moves.
  {
    RE2::Set set(options, anchor);
    for (const std::string& p : patterns) {
      std::string error;
      set.Add(p, &error);
      re2_filtered_set_sink += static_cast<uint8_t>(error.size());
    }
    if (set.Compile()) {
      std::vector<int> v;
      set.Match(text, &v);
      re2_filtered_set_sink += static_cast<uint8_t>(v.size());
      // The bool-only form: caller does not care which patterns matched.
      set.Match(text, NULL);
      // The ErrorInfo form.
      RE2::Set::ErrorInfo error_info;
      set.Match(text, &v, &error_info);
      re2_filtered_set_sink += static_cast<uint8_t>(error_info.kind);
      // Move semantics: exercise the move constructor and move assignment.
      RE2::Set moved(std::move(set));
      moved.Match(text, &v);
      RE2::Set assigned(options, anchor);
      assigned = std::move(moved);
      assigned.Match(text, &v);
    }
  }

  // Part 2: FilteredRE2 with realistic atom feedback (production usage:
  // report only the atoms that actually match the text) plus the
  // over-reporting and no-match directions.
  {
    re2::FilteredRE2 filter;
    std::vector<int> ids;
    for (const std::string& p : patterns) {
      int id = -1;
      filter.Add(p, options, &id);
      if (id >= 0)
        ids.push_back(id);
    }
    std::vector<std::string> atoms;
    filter.Compile(&atoms);
    const size_t atom_limit = atoms.size() < 64 ? atoms.size() : 64;
    std::vector<int> matched_atoms;
    for (size_t ai = 0; ai < atom_limit; ai++) {
      RE2 atom_re(atoms[ai], options);
      if (atom_re.ok() && RE2::PartialMatch(text, atom_re))
        matched_atoms.push_back(static_cast<int>(ai));
    }
    std::vector<int> matches;
    filter.AllMatches(text, matched_atoms, &matches);
    re2_filtered_set_sink += static_cast<uint8_t>(matches.size());
    filter.FirstMatch(text, matched_atoms);
    std::vector<int> potentials;
    filter.AllPotentials(matched_atoms, &potentials);
    re2_filtered_set_sink += static_cast<uint8_t>(potentials.size());
    // Over-reporting: pretend that every atom matched (the only direction
    // the shipped fuzzer exercises).
    std::vector<int> all_atoms;
    for (size_t ai = 0; ai < atoms.size(); ai++)
      all_atoms.push_back(static_cast<int>(ai));
    filter.AllMatches(text, all_atoms, &matches);
    // No atoms matched at all.
    std::vector<int> none;
    filter.AllMatches(text, none, &matches);
    filter.SlowFirstMatch(text);
  }
  return 0;
}
