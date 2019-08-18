<<<<<<< HEAD   (80f1f2 Attempt to avoid VCVARSALL.BAT breakage entirely.)
=======
#!/bin/bash
set -eux

cd git/re2

case "${KOKORO_JOB_NAME}" in
  */windows-*)
    # Pin to Visual Studio 2015, which is the minimum that we support.
    CMAKE_G_A_FLAGS=('-G' 'Visual Studio 14 2015' '-A' 'x64')
    ;;
  *)
    CMAKE_G_A_FLAGS=()
    # Work around a bug in older versions of bash. :/
    set +u
    ;;
esac

cmake -D CMAKE_BUILD_TYPE=Debug "${CMAKE_G_A_FLAGS[@]}" .
cmake --build . --config Debug --clean-first
ctest -C Debug --output-on-failure -E 'dfa|exhaustive|random'

cmake -D CMAKE_BUILD_TYPE=Release "${CMAKE_G_A_FLAGS[@]}" .
cmake --build . --config Release --clean-first
ctest -C Release --output-on-failure -E 'dfa|exhaustive|random'

exit 0
>>>>>>> CHANGE (96c354 Comment on why we pin to Visual Studio 2015.)
