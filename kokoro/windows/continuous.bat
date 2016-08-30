CD git/re2                                                          || EXIT /B 1

cmake -D CMAKE_BUILD_TYPE=Debug -A x64 .                            || EXIT /B 1
cmake --build . --config Debug --clean-first                        || EXIT /B 1
ctest --build-config Debug -E dfa^|exhaustive^|random               || EXIT /B 1

cmake -D CMAKE_BUILD_TYPE=Release -A x64 .                          || EXIT /B 1
cmake --build . --config Release --clean-first                      || EXIT /B 1
ctest --build-config Release -E dfa^|exhaustive^|random             || EXIT /B 1

EXIT /B 0
