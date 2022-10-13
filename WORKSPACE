# Copyright 2009 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Bazel (http://bazel.io/) WORKSPACE file for RE2.

workspace(name = "com_googlesource_code_re2")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-master",
    urls = ["https://github.com/abseil/abseil-cpp/archive/master.zip"],
)

http_archive(
    name = "bazel_skylib",
    strip_prefix = "bazel-skylib-main",
    urls = ["https://github.com/bazelbuild/bazel-skylib/archive/main.zip"],
)

http_archive(
    name = "com_github_google_benchmark",
    strip_prefix = "benchmark-main",
    urls = ["https://github.com/google/benchmark/archive/main.zip"],
)

http_archive(
    name = "com_google_googletest",
    strip_prefix = "googletest-main",
    urls = ["https://github.com/google/googletest/archive/main.zip"],
)

http_archive(
    name = "rules_python",
    strip_prefix = "rules_python-main",
    urls = ["https://github.com/bazelbuild/rules_python/archive/main.zip"],
)

http_archive(
    name = "io_abseil_py",
    strip_prefix = "abseil-py-main",
    urls = ["https://github.com/abseil/abseil-py/archive/main.zip"],
)

http_archive(
    name = "pybind11_bazel",
    strip_prefix = "pybind11_bazel-master",
    urls = ["https://github.com/pybind/pybind11_bazel/archive/master.zip"],
)

http_archive(
    name = "pybind11",
    build_file = "@pybind11_bazel//:pybind11.BUILD",
    strip_prefix = "pybind11-master",
    urls = ["https://github.com/pybind/pybind11/archive/master.zip"],
)

load("@pybind11_bazel//:python_configure.bzl", "python_configure")

python_configure(name = "local_config_python")
