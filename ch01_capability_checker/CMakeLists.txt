cmake_minimum_required(VERSION 3.6)
project(capability_checker C)

set(CMAKE_C_STANDARD 11)

find_package(Java REQUIRED)
find_package(JNI REQUIRED)

include_directories(
    ${JAVA_INCLUDE_PATH}
    ${JAVA_INCLUDE_PATH2}
    ${JNI_INCLUDE_DIRS}
    src
)

add_library(capability_checker SHARED src/agent.c)

set_target_properties(capability_checker PROPERTIES
    OUTPUT_NAME "capability_checker"
    PREFIX ""
)
