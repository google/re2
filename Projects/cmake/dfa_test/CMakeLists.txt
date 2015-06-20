cmake_minimum_required( VERSION 2.8.6 )

project( dfa_test CXX)

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

set (dfa_test_SOURCES
     ${CMAKE_SOURCE_DIR}/../../re2/testing/dfa_test.cc
     ${platform_SOURCES}
     )

source_group( "Source Files" FILES ${dfa_test_SOURCES})

include_directories( dfa_test
                     ${CMAKE_SOURCE_DIR}/../../re2/testing
                     ${PCRE_INC_DIR} )

link_directories( dfa_test ${PCRE_LIB_DIR}/Release )

add_executable( dfa_test ${dfa_test_SOURCES} )

target_link_libraries ( dfa_test re2 pcre pcrecpp ${WindowsOnlyLibs} ${PosixLibs} )

add_dependencies( dfa_test re2 )

