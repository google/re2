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
