cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(sz LANGUAGES C CXX)

option(BUILD_TESTS "Build tests" ON)

add_subdirectory(src)
add_subdirectory(ledger)

#add_executable(sz src/main.cpp)
#target_link_libraries(sz PRIVATE sz_lib)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
