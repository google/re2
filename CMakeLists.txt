# Copyright 2015 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# https://github.com/google/oss-policies-info/blob/main/foundational-cxx-support-matrix.md
cmake_minimum_required(VERSION 3.13)

project(RE2 CXX)
include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)

option(BUILD_SHARED_LIBS "build shared libraries" OFF)
option(RE2_USE_ICU "build against ICU for full Unicode properties support" OFF)

# For historical reasons, this is just "USEPCRE", not "RE2_USE_PCRE".
option(USEPCRE "build against PCRE for testing and benchmarking" OFF)

# See https://groups.google.com/g/re2-dev/c/P6_NM0YIWvA for details.
# This has no effect unless RE2 is being built for an Apple platform
# such as macOS or iOS.
option(RE2_BUILD_FRAMEWORK "build RE2 as a framework" OFF)

# CMake seems to have no way to enable/disable testing per subproject,
# so we provide an option similar to BUILD_TESTING, but just for RE2.
option(RE2_BUILD_TESTING "enable testing for RE2" OFF)

# The pkg-config Requires: field.
set(REQUIRES)

# ABI version
# http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html
set(SONAME 11)

set(EXTRA_TARGET_LINK_LIBRARIES)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  if(MSVC_VERSION LESS 1920)
    message(FATAL_ERROR "you need Visual Studio 2019 or later")
  endif()
  if(BUILD_SHARED_LIBS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif()
  # CMake defaults to /W3, but some users like /W4 (or /Wall) and /WX,
  # so we disable various warnings that aren't particularly helpful.
  add_compile_options(/wd4100 /wd4201 /wd4456 /wd4457 /wd4702 /wd4815)
  # Without a byte order mark (BOM), Visual Studio assumes that the source
  # file is encoded using the current user code page, so we specify UTF-8.
  add_compile_options(/utf-8)
endif()

if(WIN32)
  add_definitions(-DUNICODE -D_UNICODE -DSTRICT -DNOMINMAX)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
endif()

if(UNIX)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()

set(ABSL_DEPS
    absl_absl_check
    absl_absl_log
    absl_base
    absl_core_headers
    absl_fixed_array
    absl_flags
    absl_flat_hash_map
    absl_flat_hash_set
    absl_hash
    absl_inlined_vector
    absl_optional
    absl_span
    absl_str_format
    absl_strings
    absl_synchronization
    )

# If a top-level project has called add_directory(abseil-cpp) already (possibly
# indirectly), let that take precedence over any copy of Abseil that might have
# been installed on the system. And likewise for ICU, GoogleTest and Benchmark.
if(NOT TARGET absl::base)
  find_package(absl REQUIRED)
endif()
list(APPEND REQUIRES ${ABSL_DEPS})

if(RE2_USE_ICU)
  if(NOT TARGET ICU::uc)
    find_package(ICU REQUIRED COMPONENTS uc)
  endif()
  add_definitions(-DRE2_USE_ICU)
  list(APPEND REQUIRES icu-uc)
endif()

if(USEPCRE)
  add_definitions(-DUSEPCRE)
  list(APPEND EXTRA_TARGET_LINK_LIBRARIES pcre)
endif()

list(JOIN REQUIRES " " REQUIRES)

set(RE2_SOURCES
    re2/bitmap256.cc
    re2/bitstate.cc
    re2/compile.cc
    re2/dfa.cc
    re2/filtered_re2.cc
    re2/mimics_pcre.cc
    re2/nfa.cc
    re2/onepass.cc
    re2/parse.cc
    re2/perl_groups.cc
    re2/prefilter.cc
    re2/prefilter_tree.cc
    re2/prog.cc
    re2/re2.cc
    re2/regexp.cc
    re2/set.cc
    re2/simplify.cc
    re2/tostring.cc
    re2/unicode_casefold.cc
    re2/unicode_groups.cc
    util/rune.cc
    util/strutil.cc
    )

set(RE2_HEADERS
    re2/filtered_re2.h
    re2/re2.h
    re2/set.h
    re2/stringpiece.h
    )

add_library(re2 ${RE2_SOURCES})
target_compile_features(re2 PUBLIC cxx_std_14)
target_include_directories(re2 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
# CMake gives "set_target_properties called with incorrect number of arguments."
# errors if we don't quote ${RE2_HEADERS}, so quote it despite prevailing style.
set_target_properties(re2 PROPERTIES PUBLIC_HEADER "${RE2_HEADERS}")
set_target_properties(re2 PROPERTIES SOVERSION ${SONAME} VERSION ${SONAME}.0.0)
add_library(re2::re2 ALIAS re2)

if(APPLE AND RE2_BUILD_FRAMEWORK)
  set_target_properties(re2 PROPERTIES
                        FRAMEWORK TRUE
                        FRAMEWORK_VERSION A
                        MACOSX_FRAMEWORK_IDENTIFIER com.googlesource.code.re2)
endif()

if(UNIX)
  target_link_libraries(re2 PUBLIC Threads::Threads)
endif()

foreach(dep ${ABSL_DEPS})
  # Work around https://gitlab.kitware.com/cmake/cmake/-/issues/16899. >:(
  string(PREPEND dep "^")
  string(REGEX REPLACE "\\^absl_" "absl::" dep ${dep})
  target_link_libraries(re2 PUBLIC ${dep})
endforeach()

if(RE2_USE_ICU)
  target_link_libraries(re2 PUBLIC ICU::uc)
endif()

if(RE2_BUILD_TESTING)
  if(NOT TARGET GTest::gtest)
    find_package(GTest REQUIRED)
  endif()
  if(NOT TARGET benchmark::benchmark)
    find_package(benchmark REQUIRED)
  endif()

  set(TESTING_SOURCES
      re2/testing/backtrack.cc
      re2/testing/dump.cc
      re2/testing/exhaustive_tester.cc
      re2/testing/null_walker.cc
      re2/testing/regexp_generator.cc
      re2/testing/string_generator.cc
      re2/testing/tester.cc
      util/pcre.cc
      )

  add_library(testing ${TESTING_SOURCES})
  if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(testing PRIVATE -DRE2_BUILD_TESTING_DLL)
  endif()
  target_compile_features(testing PUBLIC cxx_std_14)
  target_link_libraries(testing PUBLIC re2 GTest::gtest)

  set(TEST_TARGETS
      charclass_test
      compile_test
      filtered_re2_test
      mimics_pcre_test
      parse_test
      possible_match_test
      re2_test
      re2_arg_test
      regexp_test
      required_prefix_test
      search_test
      set_test
      simplify_test
      string_generator_test

      dfa_test
      exhaustive1_test
      exhaustive2_test
      exhaustive3_test
      exhaustive_test
      random_test
      )

  set(BENCHMARK_TARGETS
      regexp_benchmark
      )

  foreach(target ${TEST_TARGETS})
    add_executable(${target} re2/testing/${target}.cc)
    if(BUILD_SHARED_LIBS AND WIN32)
      target_compile_definitions(${target} PRIVATE -DRE2_CONSUME_TESTING_DLL)
    endif()
    target_compile_features(${target} PUBLIC cxx_std_14)
    target_link_libraries(${target} PUBLIC testing GTest::gtest_main ${EXTRA_TARGET_LINK_LIBRARIES})
    add_test(NAME ${target} COMMAND ${target})
  endforeach()

  foreach(target ${BENCHMARK_TARGETS})
    add_executable(${target} re2/testing/${target}.cc)
    if(BUILD_SHARED_LIBS AND WIN32)
      target_compile_definitions(${target} PRIVATE -DRE2_CONSUME_TESTING_DLL)
    endif()
    target_compile_features(${target} PUBLIC cxx_std_14)
    target_link_libraries(${target} PUBLIC testing benchmark::benchmark_main ${EXTRA_TARGET_LINK_LIBRARIES})
  endforeach()
endif()

install(TARGETS re2
        EXPORT re2Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/re2
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT re2Targets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/re2
        NAMESPACE re2::)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/re2Config.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/re2Config.cmake
                              INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/re2)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/re2ConfigVersion.cmake
                                 VERSION ${SONAME}.0.0
                                 COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/re2Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/re2ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/re2)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/re2.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/re2.pc
               @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/re2.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
