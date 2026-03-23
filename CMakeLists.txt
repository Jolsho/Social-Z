cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(sz LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

# EXECUTABLES
file(GLOB_RECURSE SOURCES "src/**/*.cpp")
add_executable(${PROJECT_NAME} src/main.cpp src/msg.cpp src/utils.cpp ${SOURCES})

# HEADERS
target_include_directories(${PROJECT_NAME} 
    PUBLIC 
        include
    PRIVATE
        src
)

####################################################
#                   SODIUM
####################################################
set(SODIUM_DIR ${CMAKE_SOURCE_DIR}/lib/libsodium)
target_include_directories(sz PRIVATE ${SODIUM_DIR}/include)
target_link_libraries(sz PRIVATE ${SODIUM_DIR}/lib/libsodium.a)


####################################################
#                   LMDB
####################################################
find_path(LMDB_INCLUDE_DIR lmdb.h)
find_library(LMDB_LIBRARY NAMES lmdb)

if(NOT LMDB_INCLUDE_DIR OR NOT LMDB_LIBRARY)
    message(FATAL_ERROR "LMDB not found on the system")
endif()

set(LMDB_DIR ${CMAKE_SOURCE_DIR}/lib/lmdb)
target_include_directories(sz PRIVATE ${LMDB_DIR}/include)
target_link_libraries(sz PRIVATE ${LMDB_DIR}/lib/liblmdb.a)



