#!/bin/bash
set -eux

cd git/re2

cmake -D CMAKE_BUILD_TYPE=Debug .
cmake --build . --config Debug --clean-first
ctest -C Debug --output-on-failure -E dfa\|exhaustive\|random

cmake -D CMAKE_BUILD_TYPE=Release .
cmake --build . --config Release --clean-first
ctest -C Release --output-on-failure -E dfa\|exhaustive\|random

exit 0
