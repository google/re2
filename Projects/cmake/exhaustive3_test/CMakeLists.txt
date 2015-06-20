cmake_minimum_required( VERSION 2.8.6 )

project( exhaustive3_test CXX)

if ( WIN32 )
    set ( WindowsOnlyLibs )
else ()
    set ( PosixLibs  pthread )
endif( )

set (exhaustive3_test_SOURCES
     ${CMAKE_SOURCE_DIR}/../../re2/testing/backtrack.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/exhaustive3_test.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/exhaustive_tester.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/regexp_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/string_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/tester.cc
     ${CMAKE_SOURCE_DIR}/../../util/pcre.cc
     ${CMAKE_SOURCE_DIR}/../../util/random.cc
     )

source_group( "Source Files" FILES ${exhaustive3_test_SOURCES})

include_directories( exhaustive3_test
                     ${CMAKE_SOURCE_DIR}/../../re2/testing
                     ${PCRE_INC_DIR} )

link_directories( exhaustive3_test ${PCRE_LIB_DIR}/Release )

add_executable( exhaustive3_test ${exhaustive3_test_SOURCES} )

target_link_libraries ( exhaustive3_test re2 pcre pcrecpp ${WindowsOnlyLibs} ${PosixLibs} )

add_dependencies( exhaustive3_test re2 )

