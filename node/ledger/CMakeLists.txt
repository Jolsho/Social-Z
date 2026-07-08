# SPDX-License-Identifier: GPL-3.0-only
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
file(GLOB_RECURSE SRC "src/**/*.cpp")
add_library(ledger OBJECT ${SRC})
target_include_directories(ledger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(ledger PRIVATE 
    core 
    blst 
    blake3
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/sz/ledger DESTINATION include/sz/)

####################################################
#                   TESTING
####################################################
option(TESTING "Build test executables" OFF)
if(TESTING)
    add_subdirectory(tests)
endif()
