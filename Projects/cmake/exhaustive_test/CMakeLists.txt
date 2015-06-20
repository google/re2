cmake_minimum_required( VERSION 2.8.6 )

project( exhaustive_test CXX)

if ( WIN32 )
    set ( WindowsOnlyLibs )
else ()
    set ( PosixLibs  pthread )
endif( )

set (exhaustive_test_SOURCES
     ${CMAKE_SOURCE_DIR}/../../re2/testing/backtrack.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/exhaustive_test.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/exhaustive_tester.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/regexp_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/string_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/tester.cc
     ${CMAKE_SOURCE_DIR}/../../util/pcre.cc
     ${CMAKE_SOURCE_DIR}/../../util/random.cc
     )

source_group( "Source Files" FILES ${exhaustive_test_SOURCES})

include_directories( exhaustive_test
                     ${CMAKE_SOURCE_DIR}/../../re2/testing
                     ${PCRE_INC_DIR} )

link_directories( exhaustive_test ${PCRE_LIB_DIR}/Release )

add_executable( exhaustive_test ${exhaustive_test_SOURCES} )

target_link_libraries ( exhaustive_test re2 pcre pcrecpp ${WindowsOnlyLibs} ${PosixLibs} )

add_dependencies( exhaustive_test re2 )

