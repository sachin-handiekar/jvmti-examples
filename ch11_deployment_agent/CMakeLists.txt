cmake_minimum_required(VERSION 3.10)
project(prod_agent C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(prod_agent prod_agent.c)
