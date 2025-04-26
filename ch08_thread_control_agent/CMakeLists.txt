cmake_minimum_required(VERSION 3.6)
project(thread_control_agent)

set(CMAKE_C_STANDARD 99)

add_library(thread_control_agent SHARED thread_control_agent.c)

target_include_directories(thread_control_agent PRIVATE $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)
