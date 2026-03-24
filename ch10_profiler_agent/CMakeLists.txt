cmake_minimum_required(VERSION 3.10)
project(profiler_agent C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(profiler_agent profiler_agent.c)
