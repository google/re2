#!/bin/bash
set -eux

cmake_vars=(-D RE2_BUILD_TESTING=ON)
if [[ ${RUNNER_OS} == Windows ]]; then
  cmake_vars+=(-D CMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake)
fi

cmake . -D CMAKE_BUILD_TYPE=Debug "${cmake_vars[@]}"
cmake --build . --config Debug --clean-first
ctest -C Debug --output-on-failure -E 'dfa|exhaustive|random'

cmake . -D CMAKE_BUILD_TYPE=Release "${cmake_vars[@]}"
cmake --build . --config Release --clean-first
ctest -C Release --output-on-failure -E 'dfa|exhaustive|random'

exit 0
