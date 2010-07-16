// Copyright 2010 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "re2/set.h"

#include "util/util.h"
#include "re2/stringpiece.h"
#include "re2/prog.h"
#include "re2/re2.h"
#include "re2/regexp.h"

using namespace re2;

RE2::Set::Set(const RE2::Options& options, RE2::Anchor anchor) {
  options_.Copy(options);
  anchor_ = anchor;
  prog_ = NULL;
  compiled_ = false;
}

RE2::Set::~Set() {
  for (int i = 0; i < re_.size(); i++)
    re_[i]->Decref();
  delete prog_;
}

int RE2::Set::Add(const StringPiece& pattern, string* error) {
  if (compiled_) {
    LOG(DFATAL) << "RE2::Set::Add after Compile";
    return -1;
  }

  RegexpStatus status;
  re2::Regexp* re = Regexp::Parse(
    pattern,
    static_cast<Regexp::ParseFlags>(options_.ParseFlags()),
    &status);
  if (re == NULL) {
    if (error != NULL)
      *error = status.Text();
    if (options_.log_errors())
      LOG(ERROR) << "Error parsing '" << pattern << "': " << status.Text();
    return -1;
  }

  re2::Regexp* sre = re->Simplify();
  re->Decref();
  re = sre;
  if(re == NULL) {
    if (error != NULL)
      *error = "simplification failed";
    if (options_.log_errors())
      LOG(ERROR) << "Error simplifying '" << pattern << "'";
    return -1;
  }

  int n = re_.size();
  re_.push_back(re);
  return n;
}

bool RE2::Set::Compile() {
  if (compiled_) {
    LOG(DFATAL) << "RE2::Set::Compile multiple times";
    return false;
  }
  compiled_ = true;
  prog_ = Prog::CompileSet(options_, anchor_, re_);
  return prog_ != NULL;
}

bool RE2::Set::Match(const StringPiece& text, vector<int>* v) const {
  if (!compiled_) {
    LOG(DFATAL) << "RE2::Set::Match without Compile";
    return false;
  }
  v->clear();
  bool failed;
  bool ret = prog_->SearchDFA(text, text, Prog::kAnchored,
                              Prog::kManyMatch, NULL, &failed, v);
  if (ret == false)
    return false;
  if (v->size() == 0) {
    LOG(DFATAL) << "RE2::Set::Match: match but unknown regexp set";
    return false;
  }
  return true;
}
