CD git/re2 ^
  || EXIT /B 1

bazel clean ^
  || EXIT /B 1
bazel build --compilation_mode=dbg -- //... ^
  || EXIT /B 1
bazel test  --compilation_mode=dbg --test_output=errors -- //... ^
  -//:dfa_test ^
  -//:exhaustive1_test ^
  -//:exhaustive2_test ^
  -//:exhaustive3_test ^
  -//:exhaustive_test ^
  -//:random_test ^
  || EXIT /B 1

bazel clean ^
  || EXIT /B 1
bazel build --compilation_mode=opt -- //... ^
  || EXIT /B 1
bazel test  --compilation_mode=opt --test_output=errors -- //... ^
  -//:dfa_test ^
  -//:exhaustive1_test ^
  -//:exhaustive2_test ^
  -//:exhaustive3_test ^
  -//:exhaustive_test ^
  -//:random_test ^
  || EXIT /B 1

EXIT /B 0
