cmake_minimum_required(VERSION 3.10)
project(class_transform_agent C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(class_transform_agent class_transform_agent.c)
