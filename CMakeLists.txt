cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0126 NEW)

project(libsz LANGUAGES C CXX)

add_subdirectory(common)

option(BUILD_NODE       "BUILD NODE"        ON)
option(BUILD_CLIENT     "BUILD CLIENT"      ON)

if(BUILD_NODE)
    add_subdirectory(node)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()


