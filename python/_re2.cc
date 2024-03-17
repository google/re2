// Copyright 2019 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "absl/strings/string_view.h"
#include "re2/filtered_re2.h"
#include "re2/re2.h"
#include "re2/set.h"

#ifdef _WIN32
#include <basetsd.h>
#define ssize_t SSIZE_T
#endif

namespace re2_python {

// This is conventional.
namespace py = pybind11;

// In terms of the pybind11 API, a py::buffer is merely a py::object that
// supports the buffer interface/protocol and you must explicitly request
// a py::buffer_info in order to access the actual bytes. Under the hood,
// the py::buffer_info manages a reference count to the py::buffer, so it
// must be constructed and subsequently destructed while holding the GIL.
static inline absl::string_view FromBytes(const py::buffer_info& bytes) {
  char* data = reinterpret_cast<char*>(bytes.ptr);
  ssize_t size = bytes.size;
  return absl::string_view(data, size);
}

static inline int OneCharLen(const char* ptr) {
  return "\1\1\1\1\1\1\1\1\1\1\1\1\2\2\3\4"[(*ptr & 0xFF) >> 4];
}

// Helper function for when Python encodes str to bytes and then needs to
// convert str offsets to bytes offsets. Assumes that text is valid UTF-8.
ssize_t CharLenToBytes(py::buffer buffer, ssize_t pos, ssize_t len) {
  auto bytes = buffer.request();
  auto text = FromBytes(bytes);
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
ssize_t BytesToCharLen(py::buffer buffer, ssize_t pos, ssize_t endpos) {
  auto bytes = buffer.request();
  auto text = FromBytes(bytes);
  auto ptr = text.data() + pos;
  auto end = text.data() + endpos;
  ssize_t len = 0;
  while (ptr < end) {
    ptr += OneCharLen(ptr);
    ++len;
  }
  return len;
}

std::unique_ptr<RE2> RE2InitShim(py::buffer buffer,
                                 const RE2::Options& options) {
  auto bytes = buffer.request();
  auto pattern = FromBytes(bytes);
  return std::make_unique<RE2>(pattern, options);
}

py::bytes RE2ErrorShim(const RE2& self) {
  // Return std::string as bytes. That is, without decoding to str.
  return self.error();
}

std::vector<std::pair<py::bytes, int>> RE2NamedCapturingGroupsShim(
    const RE2& self) {
  const int num_groups = self.NumberOfCapturingGroups();
  std::vector<std::pair<py::bytes, int>> groups;
  groups.reserve(num_groups);
  for (const auto& it : self.NamedCapturingGroups()) {
    groups.emplace_back(it.first, it.second);
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

std::tuple<bool, py::bytes, py::bytes> RE2PossibleMatchRangeShim(
    const RE2& self, int maxlen) {
  std::string min, max;
  // Return std::string as bytes. That is, without decoding to str.
  return {self.PossibleMatchRange(&min, &max, maxlen), min, max};
}

std::vector<std::pair<ssize_t, ssize_t>> RE2MatchShim(const RE2& self,
                                                      RE2::Anchor anchor,
                                                      py::buffer buffer,
                                                      ssize_t pos,
                                                      ssize_t endpos) {
  auto bytes = buffer.request();
  auto text = FromBytes(bytes);
  const int num_groups = self.NumberOfCapturingGroups() + 1;  // need $0
  std::vector<absl::string_view> groups;
  groups.resize(num_groups);
  py::gil_scoped_release release_gil;
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

py::bytes RE2QuoteMetaShim(py::buffer buffer) {
  auto bytes = buffer.request();
  auto pattern = FromBytes(bytes);
  // Return std::string as bytes. That is, without decoding to str.
  return RE2::QuoteMeta(pattern);
}

class Set {
 public:
  Set(RE2::Anchor anchor, const RE2::Options& options)
      : set_(options, anchor) {}

  ~Set() = default;

  // Not copyable or movable.
  Set(const Set&) = delete;
  Set& operator=(const Set&) = delete;

  int Add(py::buffer buffer) {
    auto bytes = buffer.request();
    auto pattern = FromBytes(bytes);
    int index = set_.Add(pattern, /*error=*/NULL);  // -1 on error
    return index;
  }

  bool Compile() {
    // Compiling can fail.
    return set_.Compile();
  }

  std::vector<int> Match(py::buffer buffer) const {
    auto bytes = buffer.request();
    auto text = FromBytes(bytes);
    std::vector<int> matches;
    py::gil_scoped_release release_gil;
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

  int Add(py::buffer buffer, const RE2::Options& options) {
    auto bytes = buffer.request();
    auto pattern = FromBytes(bytes);
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
        py::pybind11_fail("set_->Add() failed");
      }
    }
    // Compiling can fail.
    return set_->Compile();
  }

  std::vector<int> Match(py::buffer buffer, bool potential) const {
    if (set_ == nullptr) {
      py::pybind11_fail("Match() called before compiling");
    }

    auto bytes = buffer.request();
    auto text = FromBytes(bytes);
    std::vector<int> atoms;
    py::gil_scoped_release release_gil;
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

PYBIND11_MODULE(_re2, module) {
  // Translate exceptions thrown by py::pybind11_fail() into Python.
  py::register_local_exception<std::runtime_error>(module, "Error");

  module.def("CharLenToBytes", &CharLenToBytes);
  module.def("BytesToCharLen", &BytesToCharLen);

  // CLASSES
  //     class RE2
  //         enum Anchor
  //         class Options
  //             enum Encoding
  //     class Set
  //     class Filter
  py::class_<RE2> re2(module, "RE2");
  py::enum_<RE2::Anchor> anchor(re2, "Anchor");
  py::class_<RE2::Options> options(re2, "Options");
  py::enum_<RE2::Options::Encoding> encoding(options, "Encoding");
  py::class_<Set> set(module, "Set");
  py::class_<Filter> filter(module, "Filter");

  anchor.value("UNANCHORED", RE2::Anchor::UNANCHORED);
  anchor.value("ANCHOR_START", RE2::Anchor::ANCHOR_START);
  anchor.value("ANCHOR_BOTH", RE2::Anchor::ANCHOR_BOTH);

  encoding.value("UTF8", RE2::Options::Encoding::EncodingUTF8);
  encoding.value("LATIN1", RE2::Options::Encoding::EncodingLatin1);

  options.def(py::init<>())
      .def_property("max_mem",                          //
                    &RE2::Options::max_mem,             //
                    &RE2::Options::set_max_mem)         //
      .def_property("encoding",                         //
                    &RE2::Options::encoding,            //
                    &RE2::Options::set_encoding)        //
      .def_property("posix_syntax",                     //
                    &RE2::Options::posix_syntax,        //
                    &RE2::Options::set_posix_syntax)    //
      .def_property("longest_match",                    //
                    &RE2::Options::longest_match,       //
                    &RE2::Options::set_longest_match)   //
      .def_property("log_errors",                       //
                    &RE2::Options::log_errors,          //
                    &RE2::Options::set_log_errors)      //
      .def_property("literal",                          //
                    &RE2::Options::literal,             //
                    &RE2::Options::set_literal)         //
      .def_property("never_nl",                         //
                    &RE2::Options::never_nl,            //
                    &RE2::Options::set_never_nl)        //
      .def_property("dot_nl",                           //
                    &RE2::Options::dot_nl,              //
                    &RE2::Options::set_dot_nl)          //
      .def_property("never_capture",                    //
                    &RE2::Options::never_capture,       //
                    &RE2::Options::set_never_capture)   //
      .def_property("case_sensitive",                   //
                    &RE2::Options::case_sensitive,      //
                    &RE2::Options::set_case_sensitive)  //
      .def_property("perl_classes",                     //
                    &RE2::Options::perl_classes,        //
                    &RE2::Options::set_perl_classes)    //
      .def_property("word_boundary",                    //
                    &RE2::Options::word_boundary,       //
                    &RE2::Options::set_word_boundary)   //
      .def_property("one_line",                         //
                    &RE2::Options::one_line,            //
                    &RE2::Options::set_one_line);       //

  re2.def(py::init(&RE2InitShim))
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

  set.def(py::init<RE2::Anchor, const RE2::Options&>())
      .def("Add", &Set::Add)
      .def("Compile", &Set::Compile)
      .def("Match", &Set::Match);

  filter.def(py::init<>())
      .def("Add", &Filter::Add)
      .def("Compile", &Filter::Compile)
      .def("Match", &Filter::Match)
      .def("GetRE2", &Filter::GetRE2,
           py::return_value_policy::reference_internal);
}

}  // namespace re2_python
