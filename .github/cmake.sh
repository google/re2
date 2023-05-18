#!/bin/bash
set -eux

cmake . -D CMAKE_BUILD_TYPE=Debug "$@"
cmake --build . --config Debug --clean-first
# TODO: Build the testing for RE2.
# ctest -C Debug --output-on-failure -E 'dfa|exhaustive|random'

cmake . -D CMAKE_BUILD_TYPE=Release "$@"
cmake --build . --config Release --clean-first
# TODO: Build the testing for RE2.
# ctest -C Release --output-on-failure -E 'dfa|exhaustive|random'

exit 0
