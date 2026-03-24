cmake_minimum_required(VERSION 3.10)
project(ch08_exception_agent C)

include(${CMAKE_SOURCE_DIR}/../cmake/JvmtiCommon.cmake)

add_library(exception_agent SHARED exception_agent.c)
target_include_directories(exception_agent PRIVATE
    ${JNI_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../common
)
