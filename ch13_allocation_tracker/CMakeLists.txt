cmake_minimum_required(VERSION 3.10)
project(ch13_allocation_tracker C)

include(${CMAKE_SOURCE_DIR}/../cmake/JvmtiCommon.cmake)

add_library(alloc_tracker SHARED alloc_tracker.c)
target_include_directories(alloc_tracker PRIVATE
    ${JNI_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../common
)
