cmake_minimum_required(VERSION 3.6)
project(heap_stack_agent)

set(CMAKE_C_STANDARD 99)

add_library(heap_stack_agent SHARED heap_stack_agent.c)

target_include_directories(heap_stack_agent PRIVATE $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)
