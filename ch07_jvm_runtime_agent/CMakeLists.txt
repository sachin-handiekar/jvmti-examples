cmake_minimum_required(VERSION 3.10)
project(ch07_jvm_runtime_agent C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(jvm_runtime_agent jvm_runtime_agent.c)
