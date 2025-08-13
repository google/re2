#!/bin/bash
set -eux

# Disable MSYS/MSYS2 path conversion, which interferes with Bazel.
export MSYS_NO_PATHCONV='1'
export MSYS2_ARG_CONV_EXCL='*'

for compilation_mode in dbg opt
do
  bazel clean
  bazel build \
    --extra_toolchains=//python/toolchains:all \
    --compilation_mode=${compilation_mode} -- \
    //:re2 \
    //python:re2
  bazel test \
    --extra_toolchains=//python/toolchains:all \
    --compilation_mode=${compilation_mode} -- \
    //:small_tests \
    //python:all
done

exit 0
