cmake_minimum_required( VERSION 2.8.6 )

project( exhaustive1_test CXX)

if ( WIN32 )
    set ( WindowsOnlyLibs )
else ()
    set ( PosixLibs  pthread )
endif( )

set (exhaustive1_test_SOURCES
     ${CMAKE_SOURCE_DIR}/../../re2/testing/backtrack.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/exhaustive1_test.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/exhaustive_tester.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/regexp_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/string_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/tester.cc
     ${CMAKE_SOURCE_DIR}/../../util/pcre.cc
     ${CMAKE_SOURCE_DIR}/../../util/random.cc
     )

source_group( "Source Files" FILES ${exhaustive1_test_SOURCES})

include_directories( exhaustive1_test
                     ${CMAKE_SOURCE_DIR}/../../re2/testing
                     ${PCRE_INC_DIR} )

link_directories( exhaustive1_test ${PCRE_LIB_DIR}/Release )

add_executable( exhaustive1_test ${exhaustive1_test_SOURCES} )

target_link_libraries ( exhaustive1_test re2 pcre pcrecpp ${WindowsOnlyLibs} ${PosixLibs} )

add_dependencies( exhaustive1_test re2 )

