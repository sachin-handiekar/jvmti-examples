cmake_minimum_required(VERSION 3.10)
project(event_logger_agent C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(event_logger_agent event_logger_agent.c)
