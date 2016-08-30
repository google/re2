CD git/re2                              || EXIT /B 1

cmake .                                 || EXIT /B 1
cmake --build . --clean-first           || EXIT /B 1
ctest -E dfa^|exhaustive^|random        || EXIT /B 1

cmake -DCMAKE_BUILD_TYPE=Release .      || EXIT /B 1
cmake --build . --clean-first           || EXIT /B 1
ctest -E dfa^|exhaustive^|random        || EXIT /B 1

EXIT /B 0
