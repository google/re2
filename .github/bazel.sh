#!/bin/bash
set -eux

# macOS has GNU bash 3.x, so ${RUNNER_OS,,} isn't supported.
config=--config=$(echo ${RUNNER_OS} | tr '[:upper:]' '[:lower:]')

bazel clean
bazel build ${config} --compilation_mode=dbg -- //:all
bazel test  ${config} --compilation_mode=dbg -- //:all \
  -//:dfa_test \
  -//:exhaustive1_test \
  -//:exhaustive2_test \
  -//:exhaustive3_test \
  -//:exhaustive_test \
  -//:random_test

bazel clean
bazel build ${config} --compilation_mode=opt -- //:all
bazel test  ${config} --compilation_mode=opt -- //:all \
  -//:dfa_test \
  -//:exhaustive1_test \
  -//:exhaustive2_test \
  -//:exhaustive3_test \
  -//:exhaustive_test \
  -//:random_test

exit 0
