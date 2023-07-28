#!/bin/bash
set -eux

bazelisk clean
bazelisk build --compilation_mode=dbg -- //:all
bazelisk test  --compilation_mode=dbg -- //:all \
  -//:dfa_test \
  -//:exhaustive1_test \
  -//:exhaustive2_test \
  -//:exhaustive3_test \
  -//:exhaustive_test \
  -//:random_test

bazelisk clean
bazelisk build --compilation_mode=opt -- //:all
bazelisk test  --compilation_mode=opt -- //:all \
  -//:dfa_test \
  -//:exhaustive1_test \
  -//:exhaustive2_test \
  -//:exhaustive3_test \
  -//:exhaustive_test \
  -//:random_test

exit 0
