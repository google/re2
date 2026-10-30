// Copyright 2019 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stddef.h>
#include <sys/types.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"
#include "nanobind/nanobind.h"
#include "nanobind/stl/pair.h"      // IWYU pragma: keep
#include "nanobind/stl/tuple.h"     // IWYU pragma: keep
#include "nanobind/stl/unique_ptr.h"  // IWYU pragma: keep
#include "nanobind/stl/vector.h"    // IWYU pragma: keep
#include "re2/filtered_re2.h"
#include "re2/re2.h"
#include "re2/set.h"

#ifdef _WIN32
#include <basetsd.h>
#define ssize_t SSIZE_T
#endif

namespace re2_python {

// This is conventional.
namespace nb = nanobind;

// nanobind doesn't provide a native way to access a Python buffer, so
// we extract it ourself. We use PyBUF_SIMPLE to extract a contiguous
// buffer to match the semantics of the previous pybind11-based implementation.
class BufferView {
 public:
  explicit BufferView(nb::handle obj) {
    if (PyObject_GetBuffer(obj.ptr(), &view_, PyBUF_SIMPLE) != 0) {
      throw nb::python_error();
    }
  }

  ~BufferView() { PyBuffer_Release(&view_); }

  // Not copyable or movable.
  BufferView(const BufferView&) = delete;
  BufferView& operator=(const BufferView&) = delete;

  absl::string_view view() const {
    return absl::string_view(reinterpret_cast<char*>(view_.buf),
                             static_cast<size_t>(view_.len));
  }

 private:
  Py_buffer view_;
};

static inline int OneCharLen(const char* ptr) {
  return "\1\1\1\1\1\1\1\1\1\1\1\1\2\2\3\4"[(*ptr & 0xFF) >> 4];
}

// Helper function for when Python encodes str to bytes and then needs to
// convert str offsets to bytes offsets. Assumes that text is valid UTF-8.
ssize_t CharLenToBytes(nb::object buffer, ssize_t pos, ssize_t len) {
  BufferView bytes(buffer);
  auto text = bytes.view();
  auto ptr = text.data() + pos;
  auto end = text.data() + text.size();
  while (ptr < end && len > 0) {
    ptr += OneCharLen(ptr);
    --len;
  }
  return ptr - (text.data() + pos);
}

// Helper function for when Python decodes bytes to str and then needs to
// convert bytes offsets to str offsets. Assumes that text is valid UTF-8.
ssize_t BytesToCharLen(nb::object buffer, ssize_t pos, ssize_t endpos) {
  BufferView bytes(buffer);
  auto text = bytes.view();
  auto ptr = text.data() + pos;
  auto end = text.data() + endpos;
  ssize_t len = 0;
  while (ptr < end) {
    ptr += OneCharLen(ptr);
    ++len;
  }
  return len;
}

std::unique_ptr<RE2> RE2InitShim(nb::object buffer,
                                 const RE2::Options& options) {
  BufferView bytes(buffer);
  auto pattern = bytes.view();
  return std::make_unique<RE2>(pattern, options);
}

nb::bytes RE2ErrorShim(const RE2& self) {
  // Return std::string as bytes. That is, without decoding to str.
  const std::string& error = self.error();
  return nb::bytes(error.data(), error.size());
}

std::vector<std::pair<nb::bytes, int>> RE2NamedCapturingGroupsShim(
    const RE2& self) {
  const int num_groups = self.NumberOfCapturingGroups();
  std::vector<std::pair<nb::bytes, int>> groups;
  groups.reserve(num_groups);
  for (const auto& it : self.NamedCapturingGroups()) {
    groups.emplace_back(nb::bytes(it.first.data(), it.first.size()), it.second);
  }
  return groups;
}

std::vector<int> RE2ProgramFanoutShim(const RE2& self) {
  std::vector<int> histogram;
  self.ProgramFanout(&histogram);
  return histogram;
}

std::vector<int> RE2ReverseProgramFanoutShim(const RE2& self) {
  std::vector<int> histogram;
  self.ReverseProgramFanout(&histogram);
  return histogram;
}

std::tuple<bool, nb::bytes, nb::bytes> RE2PossibleMatchRangeShim(
    const RE2& self, int maxlen) {
  std::string min, max;
  bool ok = self.PossibleMatchRange(&min, &max, maxlen);
  // Return std::string as bytes. That is, without decoding to str.
  return {ok, nb::bytes(min.data(), min.size()),
          nb::bytes(max.data(), max.size())};
}

std::vector<std::pair<ssize_t, ssize_t>> RE2MatchShim(const RE2& self,
                                                      RE2::Anchor anchor,
                                                      nb::object buffer,
                                                      ssize_t pos,
                                                      ssize_t endpos) {
  BufferView bytes(buffer);
  auto text = bytes.view();
  const int num_groups = self.NumberOfCapturingGroups() + 1;  // need $0
  std::vector<absl::string_view> groups;
  groups.resize(num_groups);
  nb::gil_scoped_release release_gil;
  if (!self.Match(text, pos, endpos, anchor, groups.data(), groups.size())) {
    // Ensure that groups are null before converting to spans!
    for (auto& it : groups) {
      it = absl::string_view();
    }
  }
  std::vector<std::pair<ssize_t, ssize_t>> spans;
  spans.reserve(num_groups);
  for (const auto& it : groups) {
    if (it.data() == NULL) {
      spans.emplace_back(-1, -1);
    } else {
      spans.emplace_back(it.data() - text.data(),
                         it.data() - text.data() + it.size());
    }
  }
  return spans;
}

nb::bytes RE2QuoteMetaShim(nb::object buffer) {
  BufferView bytes(buffer);
  auto pattern = bytes.view();
  // Return std::string as bytes. That is, without decoding to str.
  std::string quoted = RE2::QuoteMeta(pattern);
  return nb::bytes(quoted.data(), quoted.size());
}

class Set {
 public:
  Set(RE2::Anchor anchor, const RE2::Options& options)
      : set_(options, anchor) {}

  ~Set() = default;

  // Not copyable or movable.
  Set(const Set&) = delete;
  Set& operator=(const Set&) = delete;

  int Add(nb::object buffer) {
    BufferView bytes(buffer);
    auto pattern = bytes.view();
    int index = set_.Add(pattern, /*error=*/NULL);  // -1 on error
    return index;
  }

  bool Compile() {
    // Compiling can fail.
    return set_.Compile();
  }

  std::vector<int> Match(nb::object buffer) const {
    BufferView bytes(buffer);
    auto text = bytes.view();
    std::vector<int> matches;
    nb::gil_scoped_release release_gil;
    set_.Match(text, &matches);
    return matches;
  }

 private:
  RE2::Set set_;
};

class Filter {
 public:
  Filter() = default;
  ~Filter() = default;

  // Not copyable or movable.
  Filter(const Filter&) = delete;
  Filter& operator=(const Filter&) = delete;

  int Add(nb::object buffer, const RE2::Options& options) {
    BufferView bytes(buffer);
    auto pattern = bytes.view();
    int index = -1;  // not clobbered on error
    filter_.Add(pattern, options, &index);
    return index;
  }

  bool Compile() {
    std::vector<std::string> atoms;
    filter_.Compile(&atoms);
    RE2::Options options;
    options.set_literal(true);
    options.set_case_sensitive(false);
    set_ = std::make_unique<RE2::Set>(options, RE2::UNANCHORED);
    for (int i = 0; i < static_cast<int>(atoms.size()); ++i) {
      if (set_->Add(atoms[i], /*error=*/NULL) != i) {
        // Should never happen: the atom is a literal!
        throw std::runtime_error("set_->Add() failed");
      }
    }
    // Compiling can fail.
    return set_->Compile();
  }

  std::vector<int> Match(nb::object buffer, bool potential) const {
    if (set_ == nullptr) {
      throw std::runtime_error("Match() called before compiling");
    }

    BufferView bytes(buffer);
    auto text = bytes.view();
    std::vector<int> atoms;
    nb::gil_scoped_release release_gil;
    set_->Match(text, &atoms);
    std::vector<int> matches;
    if (potential) {
      filter_.AllPotentials(atoms, &matches);
    } else {
      filter_.AllMatches(text, atoms, &matches);
    }
    return matches;
  }

  const RE2& GetRE2(int index) const {
    return filter_.GetRE2(index);
  }

 private:
  re2::FilteredRE2 filter_;
  std::unique_ptr<RE2::Set> set_;
};

NB_MODULE(_re2, module) {
  // Translate exceptions thrown by throw std::runtime_error() into Python.
  nb::exception<std::runtime_error>(module, "Error");

  module.def("CharLenToBytes", &CharLenToBytes);
  module.def("BytesToCharLen", &BytesToCharLen);

  // CLASSES
  //     class RE2
  //         enum Anchor
  //         class Options
  //             enum Encoding
  //     class Set
  //     class Filter
  nb::class_<RE2> re2(module, "RE2");
  nb::enum_<RE2::Anchor> anchor(re2, "Anchor");
  nb::class_<RE2::Options> options(re2, "Options");
  nb::enum_<RE2::Options::Encoding> encoding(options, "Encoding");
  nb::class_<Set> set(module, "Set");
  nb::class_<Filter> filter(module, "Filter");

  anchor.value("UNANCHORED", RE2::Anchor::UNANCHORED);
  anchor.value("ANCHOR_START", RE2::Anchor::ANCHOR_START);
  anchor.value("ANCHOR_BOTH", RE2::Anchor::ANCHOR_BOTH);

  encoding.value("UTF8", RE2::Options::Encoding::EncodingUTF8);
  encoding.value("LATIN1", RE2::Options::Encoding::EncodingLatin1);

  options.def(nb::init<>())
      .def_prop_rw("max_mem",                           //
                   &RE2::Options::max_mem,              //
                   &RE2::Options::set_max_mem)          //
      .def_prop_rw("encoding",                          //
                   &RE2::Options::encoding,             //
                   &RE2::Options::set_encoding)         //
      .def_prop_rw("posix_syntax",                      //
                   &RE2::Options::posix_syntax,         //
                   &RE2::Options::set_posix_syntax)     //
      .def_prop_rw("longest_match",                     //
                   &RE2::Options::longest_match,        //
                   &RE2::Options::set_longest_match)    //
      .def_prop_rw("log_errors",                        //
                   &RE2::Options::log_errors,           //
                   &RE2::Options::set_log_errors)       //
      .def_prop_rw("literal",                           //
                   &RE2::Options::literal,              //
                   &RE2::Options::set_literal)          //
      .def_prop_rw("never_nl",                          //
                   &RE2::Options::never_nl,             //
                   &RE2::Options::set_never_nl)         //
      .def_prop_rw("dot_nl",                            //
                   &RE2::Options::dot_nl,               //
                   &RE2::Options::set_dot_nl)           //
      .def_prop_rw("never_capture",                     //
                   &RE2::Options::never_capture,        //
                   &RE2::Options::set_never_capture)    //
      .def_prop_rw("case_sensitive",                    //
                   &RE2::Options::case_sensitive,       //
                   &RE2::Options::set_case_sensitive)   //
      .def_prop_rw("perl_classes",                      //
                   &RE2::Options::perl_classes,         //
                   &RE2::Options::set_perl_classes)     //
      .def_prop_rw("word_boundary",                     //
                   &RE2::Options::word_boundary,        //
                   &RE2::Options::set_word_boundary)    //
      .def_prop_rw("one_line",                          //
                   &RE2::Options::one_line,             //
                   &RE2::Options::set_one_line);        //

  re2.def(nb::new_(&RE2InitShim))
      .def("ok", &RE2::ok)
      .def("error", &RE2ErrorShim)
      .def("options", &RE2::options)
      .def("NumberOfCapturingGroups", &RE2::NumberOfCapturingGroups)
      .def("NamedCapturingGroups", &RE2NamedCapturingGroupsShim)
      .def("ProgramSize", &RE2::ProgramSize)
      .def("ReverseProgramSize", &RE2::ReverseProgramSize)
      .def("ProgramFanout", &RE2ProgramFanoutShim)
      .def("ReverseProgramFanout", &RE2ReverseProgramFanoutShim)
      .def("PossibleMatchRange", &RE2PossibleMatchRangeShim)
      .def("Match", &RE2MatchShim)
      .def_static("QuoteMeta", &RE2QuoteMetaShim);

  set.def(nb::init<RE2::Anchor, const RE2::Options&>())
      .def("Add", &Set::Add)
      .def("Compile", &Set::Compile)
      .def("Match", &Set::Match);

  filter.def(nb::init<>())
      .def("Add", &Filter::Add)
      .def("Compile", &Filter::Compile)
      .def("Match", &Filter::Match)
      .def("GetRE2", &Filter::GetRE2, nb::rv_policy::reference_internal);
}

}  // namespace re2_python
