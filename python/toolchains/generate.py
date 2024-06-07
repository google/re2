# Copyright 2019 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

import os
import shutil
import sys
import sysconfig


def generate():
  include = sysconfig.get_path('include')
  libs = os.path.join(include, '../libs')

  mydir = os.path.dirname(sys.argv[0]) or '.'
  shutil.copytree(include, f'{mydir}/include')
  try:
    shutil.copytree(libs, f'{mydir}/libs')
  except FileNotFoundError:
    # We must not be running on Windows. :)
    pass

  with open(f'{mydir}/BUILD.bazel', 'x') as file:
    file.write(
        """\
load("@rules_python//python/cc:py_cc_toolchain.bzl", "py_cc_toolchain")
load("@rules_python//python:py_runtime.bzl", "py_runtime")
load("@rules_python//python:py_runtime_pair.bzl", "py_runtime_pair")

package(default_visibility = ["//visibility:public"])

toolchain(
    name = "py",
    toolchain = ":py_toolchain",
    toolchain_type = "@rules_python//python:toolchain_type",
)

py_runtime_pair(
    name = "py_toolchain",
    py3_runtime = ":interpreter",
)

py_runtime(
    name = "interpreter",
    interpreter_path = "{interpreter_path}",
    interpreter_version_info = {{
        "major": "{major}",
        "minor": "{minor}",
    }},
    python_version = "PY3",
)

toolchain(
    name = "py_cc",
    toolchain = ":py_cc_toolchain",
    toolchain_type = "@rules_python//python/cc:toolchain_type",
)

py_cc_toolchain(
    name = "py_cc_toolchain",
    headers = ":headers",
    libs = ":libraries",
    python_version = "{major}.{minor}",
)

cc_library(
    name = "headers",
    hdrs = glob(["include/**/*.h"]),
    includes = ["include"],
    deps = select({{
        "@platforms//os:windows": [":interface_library"],
        "//conditions:default": [],
    }}),
)

cc_import(
    name = "interface_library",
    interface_library = select({{
        "@platforms//os:windows": "libs/python{major}{minor}.lib",
        "//conditions:default": None,
    }}),
    system_provided = True,
)

# Not actually necessary for our purposes. :)
cc_library(
    name = "libraries",
)
""".format(
            interpreter_path=sys.executable.replace('\\', '/'),
            major=sys.version_info.major,
            minor=sys.version_info.minor,
        )
    )


if __name__ == '__main__':
  generate()
