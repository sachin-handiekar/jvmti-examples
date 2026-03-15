# JvmtiCommon.cmake — Shared CMake configuration for all JVMTI agent examples
#
# Usage (in each chapter's CMakeLists.txt):
#   cmake_minimum_required(VERSION 3.10)
#   project(<agent_name> C)
#   include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JvmtiCommon.cmake)
#   add_jvmti_agent(<agent_name> <source_file.c>)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Portable JNI/JVMTI header discovery — works on Linux, macOS, and Windows
find_package(JNI REQUIRED)

# Convenience function: creates a shared library target with correct includes and naming
function(add_jvmti_agent TARGET_NAME)
    # All remaining arguments are source files
    set(SOURCE_FILES ${ARGN})

    add_library(${TARGET_NAME} SHARED ${SOURCE_FILES})

    target_include_directories(${TARGET_NAME} PRIVATE
        ${JNI_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../common
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${TARGET_NAME}"
    )

    # Enable warnings for better code quality
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE /W4)
    else()
        target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()
