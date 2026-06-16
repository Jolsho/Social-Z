cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(sz LANGUAGES C CXX)

add_subdirectory(node)

option(LEDGER "Add Ledger Component" OFF)
if(LEDGER)
    add_subdirectory(ledger)
endif()

option(RPC_HTTP "Build & run RPC_HTTP" OFF)
if(RPC_HTTP)
    add_subdirectory(http)
endif()


