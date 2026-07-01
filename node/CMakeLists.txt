cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0126 NEW)

project(libsz LANGUAGES C CXX)


## COMPONENTS ##################################

option(DEFAULT      "Default Build"     ON)
option(CLIENT       "Add Client Lib"    OFF)
option(USE_UTILS    ".cpp Dev Utils"    OFF)
option(DB           "Add DB Actor"      OFF)
option(FS           "Add FS Actor"      OFF)
option(LOG          "Add LOG Actor"     OFF)
option(P2P          "Add P2P Actor"     OFF)
option(LEDGER       "Add Ledger Actor"  OFF)
option(RPC          "Add RPC Actor"     OFF)

if(DEFAULT)
    set(CLIENT      ON CACHE BOOL "" FORCE)
    set(USE_UTILS   ON CACHE BOOL "" FORCE)
    set(DB          ON CACHE BOOL "" FORCE)
    set(FS          ON CACHE BOOL "" FORCE)
    set(LOG         ON CACHE BOOL "" FORCE)
    set(P2P         ON CACHE BOOL "" FORCE)
    set(LEDGER      ON CACHE BOOL "" FORCE)
endif()

if(EMSCRIPTEN)
    set(CLIENT      ON CACHE BOOL "" FORCE)

    set(DEFAULT     OFF CACHE BOOL "" FORCE)
    set(USE_UTILS   OFF CACHE BOOL "" FORCE)
    set(DB          OFF CACHE BOOL "" FORCE)
    set(FS          OFF CACHE BOOL "" FORCE)
    set(LOG         OFF CACHE BOOL "" FORCE)
    set(P2P         OFF CACHE BOOL "" FORCE)
    set(LEDGER      OFF CACHE BOOL "" FORCE)
endif()

include(cmake/Dependencies.cmake)

## BUILDING ##############################

add_library(sz STATIC)
if(EMSCRIPTEN)
    target_link_options(sz PRIVATE
        "-sALLOW_MEMORY_GROWTH=1"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
    )
endif()

add_subdirectory(core)

function(add_feature_dir feature dir)
    if(${feature})
        add_subdirectory(${dir})
    endif()
endfunction()

add_feature_dir(CLIENT client)
add_feature_dir(DB db)
add_feature_dir(FS fs)
add_feature_dir(LOG log)
add_feature_dir(P2P p2p)
add_feature_dir(RPC rpc)
add_feature_dir(LEDGER ledger)

target_link_libraries(sz
    PRIVATE
        $<$<TARGET_EXISTS:sz::core>:sz::core>
        $<$<TARGET_EXISTS:sz::db>:sz::db>
        $<$<TARGET_EXISTS:sz::fs>:sz::fs>
        $<$<TARGET_EXISTS:sz::p2p>:sz::p2p>
        $<$<TARGET_EXISTS:sz::log>:sz::log>
)

install(EXPORT sz_targets
    FILE sz_targets.cmake
    NAMESPACE sz::
    DESTINATION lib/cmake/sz
)

install(TARGETS sz
    EXPORT sz_targets
    ARCHIVE DESTINATION lib
)

if(DEFAULT)
    add_executable(sz_exec main.cpp)
    target_link_libraries(sz_exec PRIVATE
        sz::core
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

