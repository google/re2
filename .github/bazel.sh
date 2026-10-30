#!/bin/bash
set -eux

# Disable MSYS/MSYS2 path conversion, which interferes with Bazel.
export MSYS_NO_PATHCONV='1'
export MSYS2_ARG_CONV_EXCL='*'

LIMITED_API="${PY_LIMITED_API:-unset}"

for compilation_mode in dbg opt
do
  bazel clean
  bazel build \
    --extra_toolchains=//python/toolchains:all \
    --@nanobind_bazel//:py-limited-api="${LIMITED_API}" \
    --compilation_mode=${compilation_mode} -- \
    //:re2 \
    //python:re2
  bazel test \
    --extra_toolchains=//python/toolchains:all \
    --@nanobind_bazel//:py-limited-api="${LIMITED_API}" \
    --compilation_mode=${compilation_mode} -- \
    //:small_tests \
    //python:all
done

exit 0
