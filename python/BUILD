# Copyright 2009 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Bazel (http://bazel.io/) BUILD file for RE2 Python.

load("@rules_cc//cc:defs.bzl", "cc_binary")

# For now, we just assume that the pybind11 headers are installed under a system
# directory and that you will tell Bazel where the Python headers are installed:
# bazel build $(python3-config --includes | tr ' ' '\n' | sed -e 's/^/--copt=/')
cc_binary(
    name = "_re2.so",
    srcs = ["_re2.cc"],
    copts = ["-fvisibility=hidden"],
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
