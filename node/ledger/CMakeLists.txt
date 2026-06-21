# SPDX-License-Identifier: GPL-3.0-only
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(sz_ledger SHARED)

file(GLOB_RECURSE SRC "src/**/*.cpp")
target_sources(sz_ledger PRIVATE ${SRC})
target_include_directories(sz_ledger 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/bindings
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(sz_ledger PRIVATE 
    $<$<TARGET_EXISTS:blst>:blst>
    $<$<TARGET_EXISTS:lmdb>:lmdb>
    $<$<TARGET_EXISTS:blake3>:blake3>
)

####################################################
#                   TESTING
####################################################
option(TESTING "Build test executables" OFF)
if(TESTING)
    add_subdirectory(tests)
endif()
