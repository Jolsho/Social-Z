# SPDX-License-Identifier: GPL-3.0-only
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
file(GLOB_RECURSE SRC "src/**/*.cpp")

add_library(ledger ${SRC})
add_library(sz::ledger ALIAS ledger)

target_include_directories(ledger 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bindings>
        $<INSTALL_INTERFACE:include/socialz/ledger>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ledger PRIVATE 
    $<$<TARGET_EXISTS:blst>:blst>
    $<$<TARGET_EXISTS:lmdb>:lmdb>
    $<$<TARGET_EXISTS:blake3>:blake3>
)

set_target_properties(ledger PROPERTIES
    OUTPUT_NAME "sz_ledger"
    OUTPUT_NAME_DEBUG "sz_ledger"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bindings/
    DESTINATION include/socialz/ledger
)

####################################################
#                   TESTING
####################################################
option(TESTING "Build test executables" OFF)
if(TESTING)
    add_subdirectory(tests)
endif()
