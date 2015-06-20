cmake_minimum_required( VERSION 2.8.6 )

# Name of the project
project( RE2 CXX )

# Specify the target arch when generating the makefiles.
# -DTARGET_ARCH=[win32|win64|lin32|lin64]

set_property( GLOBAL PROPERTY USE_FOLDERS ON )
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" )

# Add option for using custom manifests in Release builds. Leave it off by default.
option( USE_CUSTOM_MANIFESTS "Set to ON to use custom manifests" OFF )

if( WIN32 )
    # Unicode by default on Windows.
    add_definitions( -DUNICODE
                     -D_UNICODE
                     -DNOMINMAX
                     -DRE2_HAVE_LONGLONG=1
                     -DCOMPILER_MSVC
                     -DPCRE_STATIC=1
                     -DUSEPCRE
                     -DLOGGING=1 )
else( )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsign-compare -I. -DHAVE_PTHREAD=1 -DHAVE_RWLOCK=1" )

    # Turn on debug symbols when configured for Debug mode "cmake -DCMAKE_BUILD_TYPE=Debug"
    set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g" )

    # Turn on optimizations when configured for Release mode.
    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2" )

endif( )

# ---------- Setup output Directories -------------------------
set( PROJECT_BINARY_DIR ${CMAKE_SOURCE_DIR}/build )


if ( WIN32 OR APPLE)
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR} )
  set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR} )
  set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR} )
else( ) # Linux
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE} )
  set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE} )
  set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE} )
endif( )


# ---------- Some common definitions --------------------------
set( PCRE_INC_DIR ${CMAKE_SOURCE_DIR}/../../../pcre-7.8 )
GET_filename_component( PARENT_DIR ${CMAKE_SOURCE_DIR} PATH )
set( PCRE_LIB_DIR ${CMAKE_SOURCE_DIR}/../../../pcre-7.8 )

# Tell the compiler where else to look for include files
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/../.. )

# Specify directories in which the linker will look for libraries
link_directories( ${PROJECT_BINARY_DIR} )

# Add the various subdirectories to the build
add_subdirectory( re2 )
add_subdirectory( charclass_test )
add_subdirectory( compile_test )
add_subdirectory( dfa_test )
add_subdirectory( exhaustive_test )
add_subdirectory( exhaustive1_test )
add_subdirectory( exhaustive2_test )
add_subdirectory( exhaustive3_test )
add_subdirectory( filtered_re2_test )
add_subdirectory( mimics_pcre_test )
add_subdirectory( parse_test )
add_subdirectory( possible_match_test )
add_subdirectory( re2_arg_test )
add_subdirectory( re2_test )
add_subdirectory( regexp_benchmark )
add_subdirectory( regexp_test )
add_subdirectory( required_prefix_test )
add_subdirectory( search_test )
add_subdirectory( set_test )
add_subdirectory( simplify_test )
add_subdirectory( string_generator_test )
