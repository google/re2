cmake_minimum_required( VERSION 2.8.6 )

project( filtered_re2_test CXX)

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

set (filtered_re2_test_SOURCES
     ${CMAKE_SOURCE_DIR}/../../re2/testing/backtrack.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/dump.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/exhaustive_tester.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/filtered_re2_test.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/null_walker.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/regexp_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/string_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/tester.cc
     ${CMAKE_SOURCE_DIR}/../../util/pcre.cc
     ${CMAKE_SOURCE_DIR}/../../util/random.cc
     ${platform_SOURCES}
     )

source_group( "Source Files" FILES ${filtered_re2_test_SOURCES})

include_directories( filtered_re2_test
                     ${CMAKE_SOURCE_DIR}/../../re2/testing
                     ${PCRE_INC_DIR} )

link_directories( filtered_re2_test ${PCRE_LIB_DIR}/Release )

add_executable( filtered_re2_test ${filtered_re2_test_SOURCES} )

target_link_libraries ( filtered_re2_test re2 pcre pcrecpp ${WindowsOnlyLibs} ${PosixLibs} )

add_dependencies( filtered_re2_test re2 )

