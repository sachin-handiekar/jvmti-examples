cmake_minimum_required(VERSION 3.6)
project(class_transform_agent)

set(CMAKE_C_STANDARD 99)

add_library(class_transform_agent SHARED class_transform_agent.c)

target_include_directories(class_transform_agent PRIVATE $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)
