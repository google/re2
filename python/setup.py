# Copyright 2019 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

import os
import setuptools
import setuptools.command.build_ext
import shutil
import sys
import sysconfig

long_description = r"""A drop-in replacement for the re module.

It uses RE2 under the hood, of course, so various PCRE features
(e.g. backreferences, look-around assertions) are not supported.
See https://github.com/google/re2/wiki/Syntax for the canonical
reference, but known syntactic "gotchas" relative to Python are:

  * PCRE supports \Z and \z; RE2 supports \z; Python supports \z,
    but calls it \Z. You must rewrite \Z to \z in pattern strings.

Known differences between this module's API and the re module's API:

  * The error class does not provide any error information as attributes.
  * The Options class replaces the re module's flags with RE2's options as
    gettable/settable properties. Please see re2.h for their documentation.
  * The pattern string and the input string do not have to be the same type.
    Any str will be encoded to UTF-8.
  * The pattern string cannot be str if the options specify Latin-1 encoding.

Known issues with regard to building the C++ extension:

  * Building requires RE2 to be installed on your system.
    On Debian, for example, install the libre2-dev package.
  * Building requires pybind11 to be installed on your system OR venv.
    On Debian, for example, install the pybind11-dev package.
    For a venv, install the pybind11 package from PyPI.
  * Building on macOS is known to work, but has been known to fail.
    For example, the system Python may not know which compiler flags
    to set when building bindings for software installed by Homebrew;
    see https://docs.brew.sh/Homebrew-and-Python#brewed-python-modules.
  * Building on Windows has not been tested yet and will probably fail.
"""


class BuildExt(setuptools.command.build_ext.build_ext):

  def build_extension(self, ext):
    if 'GITHUB_ACTIONS' not in os.environ:
      return super().build_extension(ext)

    cmd = ['bazel', 'build']
    try:
      cpu = os.environ['BAZEL_CPU']
      cmd.append(f'--cpu={cpu}')
      cmd.append(f'--platforms=//python:{cpu}')
      if cpu == 'x64_x86_windows':
        # Register the local 32-bit C++ toolchain with highest priority.
        # (This is likely to break in some release of Bazel after 7.0.0,
        # but this special case can hopefully be entirely removed then.)
        cmd.append(f'--extra_toolchains=@local_config_cc//:cc-toolchain-{cpu}')
    except KeyError:
      pass
    try:
      ver = os.environ['MACOSX_DEPLOYMENT_TARGET']
      cmd.append(f'--macos_minimum_os={ver}')
    except KeyError:
      pass
    # Register the local Python toolchains with highest priority.
    self.generate_python_toolchains()
    cmd.append('--extra_toolchains=//python/toolchains:all')
    # Print debug information during toolchain resolution.
    cmd.append('--toolchain_resolution_debug=.*')
    cmd += ['--compilation_mode=opt', '--', ':all']
    self.spawn(cmd)

    # This ensures that f'_re2.{importlib.machinery.EXTENSION_SUFFIXES[0]}'
    # is the filename in the destination directory, which is what's needed.
    shutil.copyfile('../bazel-bin/python/_re2.so',
                    self.get_ext_fullpath(ext.name))

    cmd = ['bazel', 'clean', '--expunge']
    self.spawn(cmd)

  def generate_python_toolchains(self):
    include = sysconfig.get_path('include')
    libs = os.path.join(include, '../libs')

    os.makedirs('toolchains')
    shutil.copytree(include, 'toolchains/include')
    try:
      shutil.copytree(libs, 'toolchains/libs')
    except FileNotFoundError:
      # We must not be running on Windows. :)
      pass

    with open('toolchains/BUILD.bazel', 'x') as file:
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
""".format(interpreter_path=sys.executable.replace('\\', '/'),
           major=sys.version_info.major,
           minor=sys.version_info.minor))


def options():
  bdist_wheel = {}
  try:
    bdist_wheel['plat_name'] = os.environ['PLAT_NAME']
  except KeyError:
    pass
  return {'bdist_wheel': bdist_wheel}


def include_dirs():
  try:
    import pybind11
    yield pybind11.get_include()
  except ModuleNotFoundError:
    pass


ext_module = setuptools.Extension(
    name='_re2',
    sources=['_re2.cc'],
    include_dirs=list(include_dirs()),
    libraries=['re2'],
    extra_compile_args=['-fvisibility=hidden'],
)

setuptools.setup(
    name='google-re2',
    version='1.1.20240501',
    description='RE2 Python bindings',
    long_description=long_description,
    long_description_content_type='text/plain',
    author='The RE2 Authors',
    author_email='re2-dev@googlegroups.com',
    url='https://github.com/google/re2',
    py_modules=['re2'],
    ext_modules=[ext_module],
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: BSD License',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3.8',
    ],
    options=options(),
    cmdclass={'build_ext': BuildExt},
    python_requires='~=3.8',
)
