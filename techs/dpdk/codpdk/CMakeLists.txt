cmake_minimum_required (VERSION 3.0)

project(codpdk VERSION 0.1 DESCRIPTION "DPDK Coding Lib")

add_definitions('-Wall')

set(library_SOURCES
    src/ncdpdk.c
    )

add_library(codpdk SHARED
    ${library_SOURCES}
    )

target_include_directories(codpdk
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    )
