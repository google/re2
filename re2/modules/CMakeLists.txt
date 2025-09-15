cmake_minimum_required(VERSION 3.28)

add_library(re2modules)
add_library(re2::modules ALIAS re2modules)

set(RE2_MODULES
	re2.cppm
)

target_link_libraries(re2modules PUBLIC re2::re2)

if(NOT COMMAND configure_cpp_module_target)
    function(configure_cpp_module_target target)
        target_sources(${target} PUBLIC FILE_SET CXX_MODULES FILES ${RE2_MODULES})
    endfunction()
endif()

configure_cpp_module_target(re2modules)

set_target_properties(re2modules
    PROPERTIES
    VERSION ${SHARED_LIBRARY_VERSION} 
    SOVERSION ${SHARED_LIBRARY_VERSION}
    OUTPUT_NAME re2modules
    DEFINE_SYMBOL re2modules_EXPORTS
)

target_include_directories(re2modules
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(re2modules PUBLIC cxx_std_20)