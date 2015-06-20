cmake_minimum_required( VERSION 2.8.6 )

project( regexp_benchmark CXX)

if ( WIN32 )
    set (platform_SOURCES
         ${CMAKE_SOURCE_DIR}/../../util/threadwin.cc
        )
    set ( WindowsOnlyLibs )
else ()
    set (platform_SOURCES
         ${CMAKE_SOURCE_DIR}/../../util/thread.cc
        )
    set ( PosixLibs  pthread )
endif( )

set (regexp_benchmark_SOURCES
     ${CMAKE_SOURCE_DIR}/../../re2/testing/backtrack.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/dump.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/exhaustive_tester.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/null_walker.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/regexp_benchmark.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/regexp_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/string_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/tester.cc
     ${CMAKE_SOURCE_DIR}/../../util/benchmark.cc
     ${CMAKE_SOURCE_DIR}/../../util/pcre.cc
     ${CMAKE_SOURCE_DIR}/../../util/random.cc
     ${platform_SOURCES}
     )

source_group( "Source Files" FILES ${regexp_benchmark_SOURCES})

include_directories( regexp_benchmark
                     ${CMAKE_SOURCE_DIR}/../../re2/testing
                     ${PCRE_INC_DIR} )

link_directories( regexp_benchmark ${PCRE_LIB_DIR}/Release )

add_executable( regexp_benchmark ${regexp_benchmark_SOURCES} )

target_link_libraries ( regexp_benchmark re2 pcre pcrecpp ${WindowsOnlyLibs} ${PosixLibs} )

add_dependencies( regexp_benchmark re2 )

