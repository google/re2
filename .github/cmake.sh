#!/bin/bash
set -eux

for CMAKE_BUILD_TYPE in Debug Release
do
  cmake . -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -D RE2_BUILD_TESTING=ON "$@"
  cmake --build . --config ${CMAKE_BUILD_TYPE} --clean-first
  ctest -C ${CMAKE_BUILD_TYPE} --output-on-failure -E 'dfa|exhaustive|random'
done

exit 0
