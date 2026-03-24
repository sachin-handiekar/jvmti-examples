cmake_minimum_required(VERSION 3.10)
project(ch05_heap_stack_agent C)

include(${CMAKE_SOURCE_DIR}/../cmake/JvmtiCommon.cmake)

add_library(heap_stack_agent SHARED heap_stack_agent.c)
target_include_directories(heap_stack_agent PRIVATE
    ${JNI_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../common
)
