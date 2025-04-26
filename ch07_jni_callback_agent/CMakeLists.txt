cmake_minimum_required(VERSION 3.6)
project(jni_callback_agent)

set(CMAKE_C_STANDARD 99)

add_library(jni_callback_agent SHARED jni_callback_agent.c)

target_include_directories(jni_callback_agent PRIVATE $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)
