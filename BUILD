# Copyright 2009 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Bazel (http://bazel.io/) BUILD file for RE2.

licenses(["notice"])

exports_files(["LICENSE"])

cc_library(
    name = "re2",
    srcs = [
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
        "re2/prefilter.h",
        "re2/prefilter_tree.cc",
        "re2/prefilter_tree.h",
        "re2/prog.cc",
        "re2/prog.h",
        "re2/re2.cc",
        "re2/regexp.cc",
        "re2/regexp.h",
        "re2/set.cc",
        "re2/simplify.cc",
        "re2/stringpiece.cc",
        "re2/tostring.cc",
        "re2/unicode_casefold.cc",
        "re2/unicode_casefold.h",
        "re2/unicode_groups.cc",
        "re2/unicode_groups.h",
        "re2/walker-inl.h",
        "util/flags.h",
        "util/hash.cc",
        "util/logging.cc",
        "util/logging.h",
        "util/mutex.h",
        "util/rune.cc",
        "util/sparse_array.h",
        "util/sparse_set.h",
        "util/stringprintf.cc",
        "util/strutil.cc",
        "util/utf.h",
        "util/util.h",
        "util/valgrind.cc",
        "util/valgrind.h",
    ],
    hdrs = [
        "re2/filtered_re2.h",
        "re2/re2.h",
        "re2/set.h",
        "re2/stringpiece.h",
        "re2/variadic_function.h",
    ],
    copts = ["-pthread"],
    includes = ["."],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "testing",
    testonly = 1,
    srcs = [
        "re2/testing/backtrack.cc",
        "re2/testing/dump.cc",
        "re2/testing/exhaustive_tester.cc",
        "re2/testing/null_walker.cc",
        "re2/testing/regexp_generator.cc",
        "re2/testing/string_generator.cc",
        "re2/testing/tester.cc",
        "util/pcre.cc",
        "util/random.cc",
        "util/thread.cc",
    ],
    hdrs = [
        "re2/testing/exhaustive_tester.h",
        "re2/testing/regexp_generator.h",
        "re2/testing/string_generator.h",
        "re2/testing/tester.h",
        "util/pcre.h",
        "util/random.h",
        "util/thread.h",
    ],
    includes = ["."],
    deps = [":re2"],
)

cc_library(
    name = "test",
    srcs = [
        "util/test.cc",
    ],
    hdrs = [
        "util/test.h",
    ],
    includes = ["."],
    deps = [":testing"],
)

load("re2_test", "re2_test")

re2_test("charclass_test")

re2_test("compile_test")

re2_test("filtered_re2_test")

re2_test("mimics_pcre_test")

re2_test("parse_test")

re2_test("possible_match_test")

re2_test("re2_arg_test")

re2_test("re2_test")

re2_test("regexp_test")

re2_test("required_prefix_test")

re2_test("search_test")

re2_test("set_test")

re2_test("simplify_test")

re2_test("string_generator_test")

re2_test(
    "dfa_test",
    size = "large",
)

re2_test(
    "exhaustive1_test",
    size = "large",
)

re2_test(
    "exhaustive2_test",
    size = "large",
)

re2_test(
    "exhaustive3_test",
    size = "large",
)

re2_test(
    "exhaustive_test",
    size = "large",
)

re2_test(
    "random_test",
    size = "large",
)

cc_library(
    name = "benchmark",
    srcs = [
        "util/benchmark.cc",
    ],
    hdrs = [
        "util/benchmark.h",
    ],
    includes = ["."],
    deps = [":testing"],
)

cc_binary(
    name = "regexp_benchmark",
    srcs = [
        "re2/testing/regexp_benchmark.cc",
    ],
    linkopts = ["-lrt"],
    deps = [":benchmark"],
)
