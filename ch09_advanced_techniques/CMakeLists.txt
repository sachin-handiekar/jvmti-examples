cmake_minimum_required(VERSION 3.10)
project(ch09_advanced_techniques C)

include(${CMAKE_SOURCE_DIR}/../cmake/JvmtiCommon.cmake)

add_library(advanced_agent SHARED advanced_agent.c)
target_include_directories(advanced_agent PRIVATE
    ${JNI_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../common
)
