# Copyright 2019 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

import os
import re
import setuptools
import setuptools.command.build_ext
import shutil

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
    cmd.append('--extra_toolchains=//python/toolchains:all')
    cmd += ['--compilation_mode=opt', '--', ':all']
    self.spawn(cmd)

    # This ensures that f'_re2.{importlib.machinery.EXTENSION_SUFFIXES[0]}'
    # is the filename in the destination directory, which is what's needed.
    shutil.copyfile('../bazel-bin/python/_re2.so',
                    self.get_ext_fullpath(ext.name))

    cmd = ['bazel', 'clean', '--expunge']
    self.spawn(cmd)


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

# We need `re2` to be a package, not a module, because it appears that
# modules can't have `.pyi` files, so munge the module into a package.
PACKAGE = 're2'
try:
  # If we are building from the sdist, we are already in package form.
  if not os.path.exists('PKG-INFO'):
    os.makedirs(PACKAGE)
    for filename in (
        're2.py',
        # TODO(junyer): Populate as per https://github.com/google/re2/issues/496.
        # 're2.pyi',
        # '_re2.pyi',
    ):
      with open(filename, 'r') as file:
        contents = file.read()
      filename = re.sub(r'^re2(?=\.py)', '__init__', filename)
      contents = re.sub(r'^(?=import _)', 'from . ', contents, flags=re.MULTILINE)
      with open(f'{PACKAGE}/{filename}', 'x') as file:
        file.write(contents)
    # TODO(junyer): Populate as per https://github.com/google/re2/issues/496.
    # with open(f'{PACKAGE}/py.typed', 'x') as file:
    #   pass

  setuptools.setup(
      name='google-re2',
      version='1.1.20240702',
      description='RE2 Python bindings',
      long_description=long_description,
      long_description_content_type='text/plain',
      author='The RE2 Authors',
      author_email='re2-dev@googlegroups.com',
      url='https://github.com/google/re2',
      packages=[PACKAGE],
      ext_package=PACKAGE,
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
except:
  raise
else:
  # If we are building from the sdist, we are already in package form.
  if not os.path.exists('PKG-INFO'):
    shutil.rmtree(PACKAGE)
