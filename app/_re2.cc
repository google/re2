// Copyright 2022 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <memory>
#include <string>

#include <emscripten/bind.h>
#include "re2/prog.h"
#include "re2/re2.h"
#include "re2/regexp.h"

namespace re2_app {

struct Info {
  std::string pattern;
  std::string error;
  std::string prefix;
  bool prefix_foldcase = false;
  std::string accel_prefix;
  bool accel_prefix_foldcase = false;
  int num_captures;
  bool is_one_pass;
  bool can_bit_state;
  std::string bytecode;
  std::string bytemap;
};

Info GetInfo(const std::string& pattern) {
  Info info;
  info.pattern = pattern;

  RE2::Options options;
  re2::RegexpStatus status;
  re2::Regexp* regexp = re2::Regexp::Parse(
      pattern, static_cast<re2::Regexp::ParseFlags>(options.ParseFlags()),
      &status);
  if (regexp == nullptr) {
    info.error = "failed to parse pattern: " + status.Text();
    return info;
  }

  std::string prefix;
  bool prefix_foldcase;
  re2::Regexp* suffix;
  if (regexp->RequiredPrefix(&prefix, &prefix_foldcase, &suffix)) {
    info.prefix = prefix;
    info.prefix_foldcase = prefix_foldcase;
  } else {
    suffix = regexp->Incref();
  }

  std::unique_ptr<re2::Prog> prog(suffix->CompileToProg(options.max_mem()));
  if (prog == nullptr) {
    info.error = "failed to compile forward Prog";
    suffix->Decref();
    regexp->Decref();
    return info;
  }

  if (regexp->RequiredPrefixForAccel(&prefix, &prefix_foldcase)) {
    info.accel_prefix = prefix;
    info.accel_prefix_foldcase = prefix_foldcase;
  }

  info.num_captures = suffix->NumCaptures();
  info.is_one_pass = prog->IsOnePass();
  info.can_bit_state = prog->CanBitState();
  info.bytecode = prog->Dump();
  info.bytemap = prog->DumpByteMap();

  suffix->Decref();
  regexp->Decref();
  return info;
}

EMSCRIPTEN_BINDINGS(_re2) {
  emscripten::value_object<Info>("Info")
      .field("pattern", &Info::pattern)
      .field("error", &Info::error)
      .field("prefix", &Info::prefix)
      .field("prefix_foldcase", &Info::prefix_foldcase)
      .field("accel_prefix", &Info::accel_prefix)
      .field("accel_prefix_foldcase", &Info::accel_prefix_foldcase)
      .field("num_captures", &Info::num_captures)
      .field("is_one_pass", &Info::is_one_pass)
      .field("can_bit_state", &Info::can_bit_state)
      .field("bytecode", &Info::bytecode)
      .field("bytemap", &Info::bytemap);

  emscripten::function("getInfo", &GetInfo);
}

}  // namespace re2_app
