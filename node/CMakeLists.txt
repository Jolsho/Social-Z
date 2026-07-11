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
option(PROXY          "Add PROXY Actor"     OFF)

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
    set(BLAKE3_SIMD_TYPE "none" CACHE STRING "force portable build for WASM" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msimd128")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msimd128")

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
add_feature_dir(PROXY proxy)
add_feature_dir(LEDGER ledger)

if(NOT EMSCRIPTEN)
    add_library(sz STATIC
        $<TARGET_OBJECTS:db>
        $<TARGET_OBJECTS:fs>
        $<TARGET_OBJECTS:p2p>
        $<TARGET_OBJECTS:log>
    )
    target_link_libraries(sz PUBLIC 
        core
        $<$<TARGET_EXISTS:db>:db>
        $<$<TARGET_EXISTS:fs>:fs>
        $<$<TARGET_EXISTS:log>:log>
        $<$<TARGET_EXISTS:p2p>:p2p>
    )
    install(TARGETS sz
        EXPORT sz_targets
        ARCHIVE DESTINATION lib
    )

    install(EXPORT sz_targets
        FILE sz_targets.cmake
        NAMESPACE sz::
        DESTINATION lib/cmake/sz
    )

endif()

if(DEFAULT)
    add_executable(sz_exec main.cpp)
    target_link_libraries(sz_exec PRIVATE
        core
        ledger
        db
        fs
        log
        p2p
    )
    set_target_properties(sz_exec PROPERTIES
        OUTPUT_NAME "socialz"
        OUTPUT_NAME_DEBUG "socialz"
    )
    install(TARGETS sz_exec RUNTIME DESTINATION bin)
endif()

configure_file(
    cmake/sz.pc.in
    ${CMAKE_BINARY_DIR}/sz.pc
    @ONLY
)
install(
    FILES ${CMAKE_BINARY_DIR}/sz.pc
    DESTINATION lib/pkgconfig
)

