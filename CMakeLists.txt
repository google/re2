# Copyright 2015 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

# Old enough to support Ubuntu Trusty.
cmake_minimum_required(VERSION 2.8.12)

project(RE2 CXX)
include(CTest)

option(BUILD_SHARED_LIBS "build shared libraries" OFF)
option(USEPCRE "use PCRE in tests and benchmarks" OFF)

set(EXTRA_TARGET_LINK_LIBRARIES)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  if(MSVC_VERSION LESS 1800)
    message(FATAL_ERROR "you need Visual Studio 2013 or later")
  endif()
  if(BUILD_SHARED_LIBS)
    # See http://www.kitware.com/blog/home/post/939 for details.
    cmake_minimum_required(VERSION 3.4)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-std=c++11)
endif()

if(WIN32)
  add_definitions(-DUNICODE -D_UNICODE -DSTRICT -DNOMINMAX)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
elseif(UNIX)
  add_compile_options(-pthread)
  list(APPEND EXTRA_TARGET_LINK_LIBRARIES -pthread)
endif()

if(USEPCRE)
  add_definitions(-DUSEPCRE)
  list(APPEND EXTRA_TARGET_LINK_LIBRARIES pcre)
endif()

include_directories(${CMAKE_SOURCE_DIR})

set(RE2_SOURCES
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
    re2/stringpiece.cc
    re2/tostring.cc
    re2/unicode_casefold.cc
    re2/unicode_groups.cc
    util/rune.cc
    util/strutil.cc
    )

add_library(re2 ${RE2_SOURCES})

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

add_library(testing STATIC ${TESTING_SOURCES})

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
  add_executable(${target} re2/testing/${target}.cc util/test.cc)
  target_link_libraries(${target} testing re2 ${EXTRA_TARGET_LINK_LIBRARIES})
  add_test(NAME ${target} COMMAND ${target})
endforeach(target)

foreach(target ${BENCHMARK_TARGETS})
  add_executable(${target} re2/testing/${target}.cc util/benchmark.cc)
  target_link_libraries(${target} testing re2 ${EXTRA_TARGET_LINK_LIBRARIES})
endforeach(target)

set(RE2_HEADERS
    re2/filtered_re2.h
    re2/re2.h
    re2/set.h
    re2/stringpiece.h
    )

install(FILES ${RE2_HEADERS} DESTINATION include/re2)
install(TARGETS re2 ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)
