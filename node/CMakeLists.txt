cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0126 NEW)

project(sz LANGUAGES C CXX)

include(cmake/Dependencies.cmake)

## COMPONENTS ##################################

option(USE_EXEC     "Add default Exec"  ON)
option(USE_UTILS    ".cpp Dev Utils"    ON)
option(DB           "Add DB Actor"      ON)
option(FS           "Add FS Actor"      ON)
option(LOG          "Add LOG Actor"     ON)
option(P2P          "Add P2P Actor"     ON)
option(LEDGER       "Add Ledger Actor"  ON)
option(RPC          "Add RPC Actor"     OFF)

if(USE_EXEC)
    set(USE_UTILS ON CACHE BOOL "" FORCE)
    set(DB        ON CACHE BOOL "" FORCE)
    set(FS        ON CACHE BOOL "" FORCE)
    set(LOG       ON CACHE BOOL "" FORCE)
    set(P2P       ON CACHE BOOL "" FORCE)
    set(LEDGER    ON CACHE BOOL "" FORCE)
endif()

## BUILDING ##############################

add_subdirectory(api)
add_subdirectory(utils)
target_link_libraries(api PRIVATE sz::utils)

function(add_feature_dir feature dir)
    if(${feature})
        add_subdirectory(${dir})
    endif()
endfunction()

add_feature_dir(DB db)
add_feature_dir(FS fs)
add_feature_dir(LOG log)
add_feature_dir(P2P p2p)
add_feature_dir(RPC rpc)
add_feature_dir(LEDGER ledger)


install(EXPORT sz_targets
    FILE sz_targets.cmake
    NAMESPACE sz::
    DESTINATION lib/cmake/sz
)

if(USE_EXEC)
    add_executable(sz_exec main.cpp)
    target_link_libraries(sz_exec PRIVATE
        sz::api
        sz::utils
        sz::ledger
        sz::db
        sz::fs
        sz::log
        sz::p2p
    )
    set_target_properties(sz_exec PROPERTIES
        OUTPUT_NAME "socialz"
        OUTPUT_NAME_DEBUG "socialz"
    )
    install(TARGETS sz_exec RUNTIME DESTINATION bin)
endif()
