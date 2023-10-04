cmake_minimum_required(VERSION 3.10)
project(gstframeparser VERSION 1.0)

set(CMAKE_C_STANDARD 11)

set(BUILD_TYPE SHARED)

find_package(PkgConfig)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4 gstreamer-video-1.0>=1.4)
exec_program(${PKG_CONFIG_EXECUTABLE} ARGS --variable pluginsdir gstreamer-1.0 OUTPUT_VARIABLE PKG_GSTREAMER_PLUGIN_DIR)

if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    add_definitions(-g)
else()
    add_definitions(-O2)
endif()

add_library(gstframeparser ${BUILD_TYPE}
        gstframeparser.c
        gstframeparser.h)

target_link_libraries(gstframeparser
        PkgConfig::GLIB
        PkgConfig::GST
        )

install(TARGETS gstframeparser LIBRARY DESTINATION ${PKG_GSTREAMER_PLUGIN_DIR})


