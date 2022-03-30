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

# RE2 doesn't depend on this anymore, but Abseil Python still does.
# We should be able to delete this when their six.BUILD is deleted.
http_archive(
    name = "six_archive",
    build_file = "@io_abseil_py//third_party:six.BUILD",
    strip_prefix = "six-master",
    urls = ["https://github.com/benjaminp/six/archive/master.zip"],
)
