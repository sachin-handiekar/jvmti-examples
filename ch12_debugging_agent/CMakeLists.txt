cmake_minimum_required(VERSION 3.10)
project(heap_stack_agent C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(heap_stack_agent heap_stack_agent.c)
