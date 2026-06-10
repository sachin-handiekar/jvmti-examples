cmake_minimum_required(VERSION 3.10)
project(ch08_exception_agent C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(exception_agent exception_agent.c)
