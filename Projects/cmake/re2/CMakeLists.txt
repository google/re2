cmake_minimum_required( VERSION 2.8.6 )

project( re2 CXX)

if ( WIN32 )
    set ( WindowsOnlyLibs )
else ()
    set ( PosixLibs  pthread )
endif( )

set (re2_SOURCES
     ${CMAKE_SOURCE_DIR}/../../re2/bitstate.cc
     ${CMAKE_SOURCE_DIR}/../../re2/compile.cc
     ${CMAKE_SOURCE_DIR}/../../re2/dfa.cc
     ${CMAKE_SOURCE_DIR}/../../re2/filtered_re2.cc
     ${CMAKE_SOURCE_DIR}/../../re2/mimics_pcre.cc
     ${CMAKE_SOURCE_DIR}/../../re2/nfa.cc
     ${CMAKE_SOURCE_DIR}/../../re2/onepass.cc
     ${CMAKE_SOURCE_DIR}/../../re2/parse.cc
     ${CMAKE_SOURCE_DIR}/../../re2/perl_groups.cc
     ${CMAKE_SOURCE_DIR}/../../re2/prefilter.cc
     ${CMAKE_SOURCE_DIR}/../../re2/prefilter_tree.cc
     ${CMAKE_SOURCE_DIR}/../../re2/prog.cc
     ${CMAKE_SOURCE_DIR}/../../re2/re2.cc
     ${CMAKE_SOURCE_DIR}/../../re2/regexp.cc
     ${CMAKE_SOURCE_DIR}/../../re2/set.cc
     ${CMAKE_SOURCE_DIR}/../../re2/simplify.cc
     ${CMAKE_SOURCE_DIR}/../../re2/tostring.cc
     ${CMAKE_SOURCE_DIR}/../../re2/unicode_casefold.cc
     ${CMAKE_SOURCE_DIR}/../../re2/unicode_groups.cc
     ${CMAKE_SOURCE_DIR}/../../util/arena.cc
     ${CMAKE_SOURCE_DIR}/../../util/hash.cc
     ${CMAKE_SOURCE_DIR}/../../util/rune.cc
     ${CMAKE_SOURCE_DIR}/../../util/stringpiece.cc
     ${CMAKE_SOURCE_DIR}/../../util/stringprintf.cc
     ${CMAKE_SOURCE_DIR}/../../util/strutil.cc
     ${CMAKE_SOURCE_DIR}/../../util/test.cc
     ${CMAKE_SOURCE_DIR}/../../util/valgrind.cc
     )

source_group( "Source Files" FILES ${re2_SOURCES})

include_directories( re2
                     ${CMAKE_SOURCE_DIR}/../../re2
                     ${CMAKE_SOURCE_DIR}/../../util )

add_library( re2 STATIC ${re2_SOURCES} )

target_link_libraries (re2 pcre pcrecpp )
