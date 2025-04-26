cmake_minimum_required(VERSION 3.6)
project(event_logger_agent)

set(CMAKE_C_STANDARD 99)

add_library(event_logger_agent SHARED event_logger_agent.c)

target_include_directories(event_logger_agent PRIVATE $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)
