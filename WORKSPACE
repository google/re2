# Copyright 2009 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Bazel (http://bazel.io/) WORKSPACE file for RE2.

workspace(name = "com_googlesource_code_re2")
<<<<<<< HEAD   (b15818 Crudely limit the use of 'k', 'K', 's' and 'S' when fuzzing.)

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_cc",
    strip_prefix = "rules_cc-main",
    urls = ["https://github.com/bazelbuild/rules_cc/archive/main.zip"],
)

http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-master",
    urls = ["https://github.com/abseil/abseil-cpp/archive/master.zip"],
)

http_archive(
    name = "com_github_google_benchmark",
    strip_prefix = "benchmark-master",
    urls = ["https://github.com/google/benchmark/archive/master.zip"],
)

http_archive(
    name = "com_google_googletest",
    strip_prefix = "googletest-master",
    urls = ["https://github.com/google/googletest/archive/master.zip"],
)

http_archive(
    name = "rules_python",
    strip_prefix = "rules_python-main",
    urls = ["https://github.com/bazelbuild/rules_python/archive/main.zip"],
)

http_archive(
    name = "io_abseil_py",
    strip_prefix = "abseil-py-master",
    urls = ["https://github.com/abseil/abseil-py/archive/master.zip"],
)

# RE2 doesn't depend on this anymore, but Abseil Python still does.
# We should be able to delete this when their six.BUILD is deleted.
http_archive(
    name = "six_archive",
    build_file = "@io_abseil_py//third_party:six.BUILD",
    strip_prefix = "six-master",
    urls = ["https://github.com/benjaminp/six/archive/master.zip"],
)
=======
>>>>>>> CHANGE (dd5f91 Switch back to native C++ rules.)
