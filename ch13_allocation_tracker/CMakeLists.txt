cmake_minimum_required(VERSION 3.10)
project(ch13_allocation_tracker C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(alloc_tracker alloc_tracker.c)
