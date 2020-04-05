# Copyright 2019 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

import setuptools

long_description = """\
A drop-in replacement for the re module.

It uses RE2 under the hood, of course, so various PCRE features
(e.g. backreferences, look-around assertions) are not supported.

Known differences between this API and the re module's API:

  * The error class does not provide any error information as attributes.
  * The Options class replaces the re module's flags with RE2's options as
    gettable/settable properties. Please see re2.h for their documentation.
  * The pattern string and the input string do not have to be the same type.
    Any Text (unicode in Python 2, str in Python 3) will be encoded to UTF-8.
  * The pattern string cannot be Text if the options specify Latin-1 encoding.

Known issues with regard to building the C++ extension:

  * Building requires RE2 and pybind11 to be installed on your system.
    On Debian, for example, install the libre2-dev and pybind11-dev packages.
  * Building on macOS has not been tested yet and will possibly fail.
  * Building on Windows has not been tested yet and will probably fail.
"""

ext_module = setuptools.Extension(
    name='_re2',
    sources=['_re2.cc'],
    libraries=['re2'],
    extra_compile_args=['-fvisibility=hidden'],
)

setuptools.setup(
    name='google-re2',
    version='0.0.6',
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
        'Programming Language :: Python :: 3.6',
    ],
    ext_modules=[ext_module],
    py_modules=['re2'],
    python_requires='~=3.6',
    install_requires=['six'],
)
