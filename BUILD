# Bazel(http://bazel.io/) build file

licenses(["notice"])

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
    "util/arena.h",
    "util/atomicops.h",
    "util/flags.h",
    "util/logging.h",
    "util/mutex.h",
    "util/pcre.h",
    "util/random.h",
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
    "util/arena.cc",
    "util/hash.cc",
    "util/pcre.cc",
    "util/random.cc",
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
  includes = [
    ".",
  ],
  hdrs = [
    "util/test.h",
  ],
  srcs = [
    "util/test.cc",
  ],
  testonly = 1,
)

cc_library(
  name="backtrack",
  hdrs = [
  ],
  srcs = [
    "re2/testing/backtrack.cc",
  ],
  deps = [
    ":re2"
  ],
  testonly = 1,
)

cc_library(
  name="dump",
  srcs = [
    "re2/testing/dump.cc",
  ],
  deps = [
    ":re2"
  ],
  testonly = 1,
)

cc_library(
  name="regexp_generator",
  hdrs = [
    "re2/testing/regexp_generator.h",
  ],
  srcs = [
    "re2/testing/regexp_generator.cc",
  ],
  deps = [
    ":re2"
  ],
  testonly = 1,
)

cc_library(
  name="string_generator",
  hdrs = [
    "re2/testing/string_generator.h",
  ],
  srcs = [
    "re2/testing/string_generator.cc",
  ],
  deps = [
    ":re2"
  ],
  testonly = 1,
)

cc_library(
  name="tester",
  hdrs = [
    "re2/testing/tester.h",
  ],
  srcs = [
    "re2/testing/tester.cc",
  ],
  deps = [
    ":re2",
    ":backtrack",
  ],
  testonly = 1,
)

load("re2_test", "re2_test")

re2_test("charclass_test")
re2_test("compile_test")
re2_test("filtered_re2_test")
re2_test("mimics_pcre_test")
re2_test("parse_test",
         deps = [":dump"])
re2_test("possible_match_test",
         deps = [":string_generator",
                 ":regexp_generator"])
re2_test("re2_test")
re2_test("re2_arg_test")
re2_test("regexp_test")
re2_test("required_prefix_test",
         deps = [":dump"])
re2_test("search_test",
         deps = [ ":regexp_generator",
                  ":tester"])
re2_test("set_test")
re2_test("simplify_test",
         deps = [":dump"])
re2_test("string_generator_test",
         deps = [":string_generator",
                 ":regexp_generator"])

