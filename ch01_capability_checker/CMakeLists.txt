cmake_minimum_required(VERSION 3.10)
project(capability_checker C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(capability_checker capability_checker_agent.c)
