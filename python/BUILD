# Copyright 2009 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Bazel (http://bazel.io/) BUILD file for RE2 Python.

load(":py_extension.bzl", "py_extension")
load("@rules_python//python:defs.bzl", "py_library", "py_test")

py_extension(
    name = "_re2",
    srcs = ["_re2.cc"],
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
)

py_test(
    name = "re2_test",
    size = "small",
    srcs = ["re2_test.py"],
    deps = [
        ":re2",
        "@io_abseil_py//absl/testing:absltest",
        "@io_abseil_py//absl/testing:parameterized",
    ],
)
