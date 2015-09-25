# Copyright 2009 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Define a bazel macro that creates cc_test for re2.
def re2_test(name, deps=[]):
  native.cc_test(
      name=name,
      srcs=["re2/testing/%s.cc" % (name)],
      deps=[
          ":re2",
          ":test",
      ] + deps
  )
