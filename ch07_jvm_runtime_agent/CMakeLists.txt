cmake_minimum_required(VERSION 3.10)
project(ch07_jvm_runtime_agent C)

include(${CMAKE_SOURCE_DIR}/../cmake/JvmtiCommon.cmake)

add_library(jvm_runtime_agent SHARED jvm_runtime_agent.c)
target_include_directories(jvm_runtime_agent PRIVATE
    ${JNI_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../common
)
