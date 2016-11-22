CD git/re2                                                            || EXIT /B 1

cmake -D CMAKE_BUILD_TYPE=Debug -G "Visual Studio 12 2013" -A x64 .   || EXIT /B 1
cmake --build . --config Debug --clean-first                          || EXIT /B 1
ctest -C Debug --output-on-failure -E dfa^|exhaustive^|random         || EXIT /B 1

cmake -D CMAKE_BUILD_TYPE=Release -G "Visual Studio 12 2013" -A x64 . || EXIT /B 1
cmake --build . --config Release --clean-first                        || EXIT /B 1
ctest -C Release --output-on-failure -E dfa^|exhaustive^|random       || EXIT /B 1

EXIT /B 0
