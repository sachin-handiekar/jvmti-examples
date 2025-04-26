cmake_minimum_required(VERSION 3.6)
project(profiler_agent)

set(CMAKE_C_STANDARD 99)

add_library(profiler_agent SHARED profiler_agent.c)

target_include_directories(profiler_agent PRIVATE $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)
