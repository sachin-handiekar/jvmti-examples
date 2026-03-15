cmake_minimum_required(VERSION 3.10)
project(jni_callback_agent C)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)

add_jvmti_agent(jni_callback_agent jni_callback_agent.c)
