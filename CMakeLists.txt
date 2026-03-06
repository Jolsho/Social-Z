cmake_minimum_required(VERSION 3.16)

project(sz LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

# EXECUTABLES
file(GLOB_RECURSE SOURCES "src/**/*.cpp")
add_executable(${PROJECT_NAME} src/main.cpp ${SOURCES})

# HEADERS
target_include_directories(${PROJECT_NAME} 
    PUBLIC 
        include
    PRIVATE
        src
)

target_include_directories(sz PRIVATE ${SODIUM_INCLUDE_DIRS})
target_link_libraries(sz PRIVATE ${SODIUM_LIBRARIES})
target_compile_options(sz PRIVATE ${SODIUM_CFLAGS_OTHER})

set_target_properties(sz PROPERTIES
    BUILD_RPATH "/usr/local/lib"
)

