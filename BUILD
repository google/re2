# Bazel (http://bazel.io/) BUILD file for RE2.

licenses(["notice"])

# stringpiece is a standalone library so that it can be used without pulling in
# all of the other parts of RE2.
cc_library(
    name = "stringpiece",
    srcs = ["re2/stringpiece.cc"],
    hdrs = ["re2/stringpiece.h"],
    includes = ["."],
    visibility = ["//visibility:public"],
)

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
        "re2/tostring.cc",
        "re2/unicode_casefold.cc",
        "re2/unicode_casefold.h",
        "re2/unicode_groups.cc",
        "re2/unicode_groups.h",
        "re2/walker-inl.h",
        "util/atomicops.h",
        "util/flags.h",
        "util/hash.cc",
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
        "re2/variadic_function.h",
    ],
    includes = ["."],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [
        ":stringpiece",
    ],
)

cc_library(
    name = "test",
    testonly = 1,
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
    hdrs = [
        "re2/testing/regexp_generator.h",
        "re2/testing/string_generator.h",
        "re2/testing/tester.h",
        "util/pcre.h",
        "util/random.h",
        "util/test.h",
    ],
    includes = ["."],
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

# TODO: Add the "big" tests from the Makefile.
# util/thread.{cc,h} will be needed for the DFA test.
# re2/testing/exhaustive_tester.{cc,h} will be needed for the exhaustive tests.
