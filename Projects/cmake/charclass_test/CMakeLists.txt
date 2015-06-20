cmake_minimum_required( VERSION 2.8.6 )

project( charclass_test CXX)

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

set (charclass_test_SOURCES
     ${CMAKE_SOURCE_DIR}/../../re2/testing/backtrack.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/charclass_test.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/dump.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/exhaustive_tester.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/null_walker.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/regexp_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/string_generator.cc
     ${CMAKE_SOURCE_DIR}/../../re2/testing/tester.cc
     ${CMAKE_SOURCE_DIR}/../../util/pcre.cc
     ${CMAKE_SOURCE_DIR}/../../util/random.cc
     ${platform_SOURCES}
     )

source_group( "Source Files" FILES ${charclass_test_SOURCES})

include_directories( charclass_test
                     ${CMAKE_SOURCE_DIR}/../../re2/testing
                     ${PCRE_INC_DIR} )

link_directories( charclass_test ${PCRE_LIB_DIR}/Release )

add_executable( charclass_test ${charclass_test_SOURCES} )

target_link_libraries ( charclass_test re2 pcre pcrecpp ${WindowsOnlyLibs} ${PosixLibs} )

add_dependencies( charclass_test re2 )

