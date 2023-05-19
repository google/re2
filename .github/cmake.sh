#!/bin/bash
set -eux

cmake . -D CMAKE_BUILD_TYPE=Debug -D RE2_BUILD_TESTING=ON "$@"
cmake --build . --config Debug --clean-first
ctest -C Debug --output-on-failure -E 'dfa|exhaustive|random'

cmake . -D CMAKE_BUILD_TYPE=Release -D RE2_BUILD_TESTING=ON "$@"
cmake --build . --config Release --clean-first
ctest -C Release --output-on-failure -E 'dfa|exhaustive|random'

exit 0
