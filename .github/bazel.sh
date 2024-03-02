#!/bin/bash
set -eux

for compilation_mode in dbg opt
do
  bazel clean
  bazel build --compilation_mode=${compilation_mode} -- \
    //:re2 \
    //python:re2
  bazel test  --compilation_mode=${compilation_mode} -- \
    //:all \
    -//:dfa_test \
    -//:exhaustive1_test \
    -//:exhaustive2_test \
    -//:exhaustive3_test \
    -//:exhaustive_test \
    -//:random_test \
    //python:all
done

exit 0
