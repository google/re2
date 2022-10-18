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
    # We will always be setting this when we are using Bazel
    # because AppleClang never increases the `-std` version.
    if 'BAZEL_CXXOPTS' not in os.environ:
      return super().build_extension(ext)

    # For @pybind11_bazel's `python_configure()`.
    os.environ['PYTHON_BIN_PATH'] = sys.executable

    # pyformat: disable
    bazel_clean = ['bazel', 'clean']
    bazel_build = ['bazel', 'build', '--compilation_mode=opt', '--', ':all']
    bazel_test  = ['bazel', 'test',  '--compilation_mode=opt', '--test_output=errors', '--', ':all']
    # pyformat: enable

    if sysconfig.get_platform().startswith('linux-'):
      self.spawn(bazel_clean)
      self.spawn(bazel_build)
      self.spawn(bazel_test)
      # This ensures that f'_re2.{importlib.machinery.EXTENSION_SUFFIXES[0]}'
      # is the filename in the destination directory, which is what's needed.
      shutil.copyfile('../bazel-bin/python/_re2.so',
                      self.get_ext_fullpath(ext.name))
    elif sysconfig.get_platform().startswith('macosx-'):
      base_bazel_cxxopts = os.environ.get('BAZEL_CXXOPTS')
      base_bazel_linkopts = os.environ.get('BAZEL_LINKOPTS')
      for target in 'x86_64', 'arm64':
        os.environ['BAZEL_CXXOPTS'] = (
            f'{base_bazel_cxxopts}:--target={target}-apple-macosx'
            if base_bazel_cxxopts else f'--target={target}-apple-macosx')
        os.environ['BAZEL_LINKOPTS'] = (
            f'{base_bazel_linkopts}:--target={target}-apple-macosx'
            if base_bazel_linkopts else f'--target={target}-apple-macosx')
        self.spawn(bazel_clean)
        self.spawn(bazel_build)
        if target == os.uname().machine:
          self.spawn(bazel_test)
        shutil.copyfile('../bazel-bin/python/_re2.so', f'_re2.{target}.so')
      # This ensures that f'_re2.{importlib.machinery.EXTENSION_SUFFIXES[0]}'
      # is the filename in the destination directory, which is what's needed.
      self.spawn(['lipo', '-create'] +
                 [f'_re2.{target}.so' for target in 'x86_64', 'arm64'] +
                 ['-output', self.get_ext_fullpath(ext.name)])


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
    version='1.0',
    description='RE2 Python bindings',
    long_description=long_description,
    long_description_content_type='text/plain',
    url='https://github.com/google/re2',
    author='The RE2 Authors',
    author_email='re2-dev@googlegroups.com',
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: BSD License',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3.7',
    ],
    cmdclass={'build_ext': BuildExt},
    ext_modules=[ext_module],
    py_modules=['re2'],
    python_requires='~=3.7',
)
