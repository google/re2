// Copyright 2010 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "re2/set.h"

#include <stddef.h>
#include <memory>

#include "util/util.h"
#include "util/logging.h"
#include "re2/stringpiece.h"
#include "re2/prog.h"
#include "re2/re2.h"
#include "re2/regexp.h"

namespace re2 {

RE2::Set::Set(const RE2::Options& options, RE2::Anchor anchor) {
  options_.Copy(options);
  anchor_ = anchor;
  prog_ = NULL;
  compiled_ = false;
  size_ = 0;
}

RE2::Set::~Set() {
  for (size_t i = 0; i < re_.size(); i++)
    re_[i]->Decref();
  delete prog_;
}

int RE2::Set::Add(const StringPiece& pattern, string* error) {
  if (compiled_) {
    LOG(DFATAL) << "RE2::Set::Add() called after compiling";
    return -1;
  }

  Regexp::ParseFlags pf = static_cast<Regexp::ParseFlags>(
    options_.ParseFlags());

  RegexpStatus status;
  re2::Regexp* re = Regexp::Parse(pattern, pf, &status);
  if (re == NULL) {
    if (error != NULL)
      *error = status.Text();
    if (options_.log_errors())
      LOG(ERROR) << "Error parsing '" << pattern << "': " << status.Text();
    return -1;
  }

  // Concatenate with match index and push on vector.
  int n = static_cast<int>(re_.size());
  re2::Regexp* m = re2::Regexp::HaveMatch(n, pf);
  if (re->op() == kRegexpConcat) {
    int nsub = re->nsub();
    re2::Regexp** sub = new re2::Regexp*[nsub + 1];
    for (int i = 0; i < nsub; i++)
      sub[i] = re->sub()[i]->Incref();
    sub[nsub] = m;
    re->Decref();
    re = re2::Regexp::Concat(sub, nsub + 1, pf);
    delete[] sub;
  } else {
    re2::Regexp* sub[2];
    sub[0] = re;
    sub[1] = m;
    re = re2::Regexp::Concat(sub, 2, pf);
  }
  re_.push_back(re);
  return n;
}

bool RE2::Set::Compile() {
  if (compiled_) {
    LOG(DFATAL) << "RE2::Set::Compile() called more than once";
    return false;
  }
  compiled_ = true;
  size_ = static_cast<int>(re_.size());

  Regexp::ParseFlags pf = static_cast<Regexp::ParseFlags>(
    options_.ParseFlags());
  re2::Regexp* re = re2::Regexp::Alternate(const_cast<re2::Regexp**>(re_.data()),
                                           static_cast<int>(re_.size()), pf);
  re_.clear();

  prog_ = Prog::CompileSet(re, anchor_, options_.max_mem());
  re->Decref();
  return prog_ != NULL;
}

bool RE2::Set::Match(const StringPiece& text, std::vector<int>* v) const {
  if (!compiled_) {
    LOG(DFATAL) << "RE2::Set::Match() called before compiling";
    return false;
  }
  bool dfa_failed = false;
  std::unique_ptr<SparseSet> matches;
  if (v != NULL) {
    matches.reset(new SparseSet(size_));
    v->clear();
  }
  bool ret = prog_->SearchDFA(text, text, Prog::kAnchored, Prog::kManyMatch,
                              NULL, &dfa_failed, matches.get());
  if (dfa_failed) {
    if (options_.log_errors())
      LOG(ERROR) << "DFA out of memory: size " << prog_->size() << ", "
                 << "bytemap range " << prog_->bytemap_range() << ", "
                 << "list count " << prog_->list_count();
    return false;
  }
  if (ret == false)
    return false;
  if (v != NULL) {
    if (matches->empty()) {
      LOG(DFATAL) << "RE2::Set::Match() matched, but matches unknown";
      return false;
    }
    v->assign(matches->begin(), matches->end());
  }
  return true;
}

}  // namespace re2
