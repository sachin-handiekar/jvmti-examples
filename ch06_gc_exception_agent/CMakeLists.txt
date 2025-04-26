cmake_minimum_required(VERSION 3.6)
project(gc_exception_agent)

set(CMAKE_C_STANDARD 99)

add_library(gc_exception_agent SHARED gc_exception_agent.c)

target_include_directories(gc_exception_agent PRIVATE $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)
