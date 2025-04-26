cmake_minimum_required(VERSION 3.6)
project(prod_agent)

set(CMAKE_C_STANDARD 99)

add_library(prod_agent SHARED prod_agent.c)

target_include_directories(prod_agent PRIVATE $ENV{JAVA_HOME}/include $ENV{JAVA_HOME}/include/win32)
