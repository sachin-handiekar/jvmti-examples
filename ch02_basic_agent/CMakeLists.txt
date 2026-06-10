cmake_minimum_required(VERSION 3.10)
project(ch02_basic_agent C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(basic_agent basic_agent.c)
