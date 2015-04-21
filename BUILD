# Bazel(http://bazel.io/) build file

licenses(["notice"])

# Make stringpiece a standalone lib so it can be used without polling
# other parts of re2.
cc_library(
  name = "stringpiece",
  visibility = ["//visibility:public"],
  hdrs = [
    "re2/stringpiece.h",
  ],
  srcs = [
    "util/stringpiece.cc",
  ],
  includes = [
    "."
  ]
)

cc_library(
  name = "re2",
  visibility = ["//visibility:public"],
  hdrs = [
    "re2/filtered_re2.h",
    "re2/re2.h",
    "re2/set.h",
    "re2/variadic_function.h",
  ],
  deps = [
    ":stringpiece",
  ],
  includes = [
    ".",
  ],
  srcs = [
    # Internal headers
    "re2/prefilter.h",
    "re2/prefilter_tree.h",
    "re2/prog.h",
    "re2/regexp.h",
    "re2/unicode_casefold.h",
    "re2/unicode_groups.h",
    "re2/walker-inl.h",
    "util/atomicops.h",
    "util/flags.h",
    "util/logging.h",
    "util/mutex.h",
    "util/sparse_array.h",
    "util/sparse_set.h",
    "util/utf.h",
    "util/util.h",
    "util/valgrind.h",
    "re2/bitstate.cc",
    "re2/compile.cc",
    "re2/dfa.cc",
    "re2/filtered_re2.cc",
    "re2/mimics_pcre.cc",
    "re2/nfa.cc",
    "re2/onepass.cc",
    "re2/parse.cc",
    "re2/perl_groups.cc",
    "re2/prefilter.cc",
    "re2/prefilter_tree.cc",
    "re2/prog.cc",
    "re2/re2.cc",
    "re2/regexp.cc",
    "re2/set.cc",
    "re2/simplify.cc",
    "re2/tostring.cc",
    "re2/unicode_casefold.cc",
    "re2/unicode_groups.cc",
    "util/hash.cc",
    "util/rune.cc",
    "util/stringprintf.cc",
    "util/strutil.cc",
    "util/valgrind.cc",
  ],
  linkopts = [
    "-pthread"
  ]
)

# Test-only libraries
cc_library(
  name="test",
  hdrs = [
    "re2/testing/regexp_generator.h",
    "re2/testing/string_generator.h",
    "re2/testing/tester.h",
    "util/pcre.h",
    "util/random.h",
    "util/test.h",
  ],
  srcs = [
    "re2/testing/backtrack.cc",
    "re2/testing/dump.cc",
    "re2/testing/regexp_generator.cc",
    "re2/testing/string_generator.cc",
    "re2/testing/tester.cc",
    "util/pcre.cc",
    "util/random.cc",
    "util/test.cc",
  ],
  testonly = 1,
  deps = [
    ":re2",
  ],
)

load("re2_test", "re2_test")

re2_test("charclass_test")
re2_test("compile_test")
re2_test("filtered_re2_test")
re2_test("mimics_pcre_test")
re2_test("parse_test")
re2_test("possible_match_test")
re2_test("re2_test")
re2_test("re2_arg_test")
re2_test("regexp_test")
re2_test("required_prefix_test")
re2_test("search_test")
re2_test("set_test")
re2_test("simplify_test")
re2_test("string_generator_test")

# TODO: support "big" tests as per Makefile, these libraries should be
# compiled too: util/thread.{cc,h} for dfa_test. Oh, and
# re2/testing/exhaustive_tester.{cc,h} for the exhaustive tests.
