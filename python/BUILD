# Copyright 2009 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Bazel (http://bazel.io/) BUILD file for RE2 Python.

load("@local_config_re2//python:defs.bzl", "COPTS")
load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_python//python:defs.bzl", "py_library", "py_test")

cc_binary(
    name = "_re2.so",
    srcs = ["_re2.cc"],
    copts = COPTS + ["-fvisibility=hidden"],
    linkshared = True,
    linkstatic = True,
    deps = [
        "//:re2",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
    ],
)

py_library(
    name = "re2",
    srcs = ["re2.py"],
    data = [":_re2.so"],
    imports = ["."],
    visibility = ["//visibility:public"],
    deps = [
        "@six_archive//:six",
    ],
)

py_test(
    name = "re2_test",
    size = "small",
    srcs = ["re2_test.py"],
    deps = [
        ":re2",
        "@io_abseil_py//absl/testing:absltest",
        "@io_abseil_py//absl/testing:parameterized",
        "@six_archive//:six",
    ],
)
