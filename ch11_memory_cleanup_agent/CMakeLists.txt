cmake_minimum_required(VERSION 3.6)
project(memory_cleanup_agent)

set(CMAKE_C_STANDARD 99)

add_library(memory_cleanup_agent SHARED memory_cleanup_agent.c)

target_include_directories(memory_cleanup_agent PRIVATE $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)
