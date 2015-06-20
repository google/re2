cmake_minimum_required( VERSION 2.8.6 )

project( simplify_test CXX)

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

set (simplify_test_SOURCES
     ${CMAKE_SOURCE_DIR}/../../re2/testing/backtrack.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/dump.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/exhaustive_tester.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/null_walker.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/regexp_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/simplify_test.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/string_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/tester.cc
     ${CMAKE_SOURCE_DIR}/../../util/pcre.cc
     ${CMAKE_SOURCE_DIR}/../../util/random.cc
     ${platform_SOURCES}
     )

source_group( "Source Files" FILES ${simplify_test_SOURCES})

include_directories( simplify_test
                     ${CMAKE_SOURCE_DIR}/../../re2/testing
                     ${PCRE_INC_DIR} )

link_directories( simplify_test ${PCRE_LIB_DIR}/Release )

add_executable( simplify_test ${simplify_test_SOURCES} )

target_link_libraries ( simplify_test re2 pcre pcrecpp ${WindowsOnlyLibs} ${PosixLibs} )

add_dependencies( simplify_test re2 )

