cmake_minimum_required(VERSION 3.10)
project(ch02_basic_agent C)

include(${CMAKE_SOURCE_DIR}/../cmake/JvmtiCommon.cmake)

add_library(basic_agent SHARED basic_agent.c)
target_include_directories(basic_agent PRIVATE
    ${JNI_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../common
)
