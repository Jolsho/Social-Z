cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0126 NEW)

project(libsz LANGUAGES C CXX)

option(BUILD_TESTING "Build project tests" OFF)
option(SZ_TESTS_ONLY "Configure tests without application dependencies" OFF)
option(SZ_TEST_SANITIZERS "Enable address and undefined behavior checks for tests" OFF)
include(CTest)

if(SZ_TESTS_ONLY)
    if(NOT BUILD_TESTING)
        message(FATAL_ERROR "SZ_TESTS_ONLY requires BUILD_TESTING=ON")
    endif()
    include(cmake/Tests.cmake)
    return()
endif()

add_subdirectory(common)

option(BUILD_NODE       "BUILD NODE"        ON)
option(BUILD_CLIENT     "BUILD CLIENT"      ON)

if(BUILD_NODE)
    add_subdirectory(node)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(BUILD_TESTING)
    include(cmake/Tests.cmake)
endif()
