cmake_minimum_required(VERSION 3.10)
project(thread_control_agent C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(thread_control_agent thread_control_agent.c)
