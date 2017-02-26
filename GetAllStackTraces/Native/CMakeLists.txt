cmake_minimum_required(VERSION 3.3)
project(GetAllStackTraces)

set(CMAKE_CXX_STANDARD 11)

# Compiler and Linker flags

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_MAKEFILE ON)
#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
set(CMAKE_CXX_FLAGS_VALGRIND "-fno-inline")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS_VALGRIND} -Werror -Wno-missing-field-initializers -Wmissing-braces")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O4 -DNDEBUG")


find_package(JNI)

if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
endif()

set(HEADER_BASE "${CMAKE_CURRENT_SOURCE_DIR}/include")

include_directories(${JNI_INCLUDE_DIRS})
include_directories(${HEADER_BASE})

set(SOURCE_FILES library.cpp ${HEADER_BASE})

add_library(GetAllStackTraces SHARED ${SOURCE_FILES})

