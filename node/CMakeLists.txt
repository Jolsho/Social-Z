cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0126 NEW)

include(FetchContent)
include(ExternalProject)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)


project(sz LANGUAGES C CXX)

## COMPONENTS ##################################

option(USE_UTILS    ".cpp Dev Utils"    ON)
option(DB           "Add DB Actor"      ON)
option(FS           "Add FS Actor"      ON)
option(LOG          "Add LOG Actor"     ON)
option(P2P          "Add P2P Actor"     ON)
option(LEDGER       "Add Ledger Actor"  ON)

option(RPC          "Add RPC Actor"     OFF)


## DEPS ##################################

set(DEPS_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/install)

option(USE_SODIUM   "Enable libsodium"  OFF)
option(USE_LMDB     "Enable LMDB"       OFF)
option(USE_BLAKE3   "Enable BLAKE3"     OFF)
option(USE_OPENSSL  "Enable OpenSSL"    OFF)
option(USE_LLHTTP   "Enable llhttp"     OFF)
option(USE_SQLITE   "Enable SQLite"     OFF)
option(USE_SIMDJSON "Enable simdjson"   OFF)
option(USE_BLST     "Enable libblst"    OFF)

if (LEDGER) 
    set(USE_BLST        ON CACHE BOOL "" FORCE)
    set(USE_LMDB        ON CACHE BOOL "" FORCE)
    set(USE_BLAKE3      ON CACHE BOOL "" FORCE)
endif()

if (FS)
    set(USE_UTILS       ON CACHE BOOL "" FORCE)
    set(USE_LMDB        ON CACHE BOOL "" FORCE)
    set(USE_SODIUM      ON CACHE BOOL "" FORCE)
    set(USE_BLAKE3      ON CACHE BOOL "" FORCE)
endif()

if (DB)
    set(USE_UTILS       ON CACHE BOOL "" FORCE)
    set(USE_LMDB        ON CACHE BOOL "" FORCE)
    set(USE_SQLITE      ON CACHE BOOL "" FORCE)
    set(USE_SIMDJSON    ON CACHE BOOL "" FORCE)
endif()

if (P2P)
    set(USE_UTILS       ON CACHE BOOL "" FORCE)
    set(USE_SODIUM      ON CACHE BOOL "" FORCE)
endif()


if (RPC)
    set(USE_UTILS       ON CACHE BOOL "" FORCE)
    set(USE_SIMDJSON    ON CACHE BOOL "" FORCE)
    set(USE_OPENSSL     ON CACHE BOOL "" FORCE)
endif()


# SODIUM
if(USE_SODIUM)
    ExternalProject_Add(sodium
        PREFIX ${CMAKE_BINARY_DIR}/_deps/sodium
        URL https://download.libsodium.org/libsodium/releases/libsodium-1.0.21-stable.tar.gz

        CONFIGURE_COMMAND ./configure --prefix=${DEPS_INSTALL_DIR}
        BUILD_COMMAND make -j

        INSTALL_COMMAND make install
        BUILD_IN_SOURCE 1
    )

    set_target_properties(sodium PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${DEPS_INSTALL_DIR}/include
    )
endif()

# LMDB
if(USE_LMDB)
    ExternalProject_Add(lmdb_ep
        PREFIX ${CMAKE_BINARY_DIR}/_deps/lmdb

        GIT_REPOSITORY https://github.com/LMDB/lmdb.git
        GIT_TAG mdb.master3
        UPDATE_DISCONNECTED TRUE

        SOURCE_SUBDIR libraries/liblmdb

        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""

        BUILD_COMMAND make
        INSTALL_COMMAND ""
    )

    set(LMDB_SRC ${CMAKE_BINARY_DIR}/_deps/lmdb/src/lmdb_ep/libraries/liblmdb)
    add_library(lmdb STATIC IMPORTED GLOBAL)
    add_dependencies(lmdb lmdb_ep)

    set_target_properties(lmdb PROPERTIES
        IMPORTED_LOCATION ${LMDB_SRC}/liblmdb.a
        INTERFACE_INCLUDE_DIRECTORIES ${LMDB_SRC}
    )
endif()


# SQLITE
if(USE_SQLITE)

    FetchContent_Declare(
      sqlite
      URL https://www.sqlite.org/2026/sqlite-amalgamation-3510300.zip
    )
    FetchContent_MakeAvailable(sqlite)

    add_library(sqlite STATIC ${sqlite_SOURCE_DIR}/sqlite3.c)
    target_include_directories(sqlite PUBLIC ${sqlite_SOURCE_DIR})
    set_target_properties(sqlite PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${sqlite_BINARY_DIR})
endif()


# BLAKE3
if(USE_BLAKE3)
    FetchContent_Declare(blake3
        GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3.git
        GIT_TAG 1.8.5
    )
    FetchContent_GetProperties(blake3)

    if(NOT blake3_POPULATED)
        FetchContent_Populate(blake3)
        add_subdirectory(${blake3_SOURCE_DIR}/c ${blake3_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
endif()

# OPENSSL
if(USE_OPENSSL)

    ExternalProject_Add(openssl
        URL https://github.com/openssl/openssl/releases/download/openssl-3.5.5/openssl-3.5.5.tar.gz
        CONFIGURE_COMMAND
            ./Configure linux-aarch64
            --prefix=${DEPS_INSTALL_DIR}
            no-apps no-tests no-docs no-shared no-legacy no-comp no-engine no-ssl3 no-tls1 no-tls1_1 no-dso

        BUILD_COMMAND make -j
        INSTALL_COMMAND make install_sw
        BUILD_IN_SOURCE 1
    )
endif()

# LLHTTP
if(USE_LLHTTP)
    FetchContent_Declare(llhttp
        GIT_REPOSITORY https://github.com/nodejs/llhttp.git
        GIT_TAG release/v9.3.1
    )

    set(LLHTTP_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(LLHTTP_BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(llhttp)
endif()

# SIMDJSON
if(USE_SIMDJSON)
    FetchContent_Declare(simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG master
    )

    FetchContent_MakeAvailable(simdjson)
endif()

# OPENSSL
if(USE_BLST)
    ExternalProject_Add(blst_ep
        PREFIX ${CMAKE_BINARY_DIR}/_deps/blst

        GIT_REPOSITORY https://github.com/supranational/blst.git
        GIT_TAG v0.3.15

        UPDATE_DISCONNECTED TRUE
        BUILD_IN_SOURCE 1

        CONFIGURE_COMMAND ""
        BUILD_COMMAND ./build.sh
        INSTALL_COMMAND ""
    )

    set(BLST_SRC ${CMAKE_BINARY_DIR}/_deps/blst/src/blst_ep)
    add_library(blst STATIC IMPORTED GLOBAL)
    add_dependencies(blst blst_ep)

    set_target_properties(blst PROPERTIES
        IMPORTED_LOCATION ${BLST_SRC}/libblst.a
        INTERFACE_INCLUDE_DIRECTORIES ${BLST_SRC}/bindings
    )

endif()


## BUILDING ##############################

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "api/*.cpp")
add_library(sz ${CORE_SOURCES})
target_include_directories(sz 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/api/include>
        $<INSTALL_INTERFACE:include>
)

add_subdirectory(utils)

target_link_libraries(sz PRIVATE sz::utils)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/api/include/
    DESTINATION include
)
install(TARGETS sz
    EXPORT sz_targets
    ARCHIVE DESTINATION lib
)


if(LEDGER)      
    add_subdirectory(ledger) 
endif()

if(RPC)         
    add_subdirectory(rpc)  
endif()

if(DB)          
    add_subdirectory(db)    
endif()

if(FS)          
    add_subdirectory(fs)    
endif()

if(LOG)         
    add_subdirectory(log)   
endif()

if(P2P)         
    add_subdirectory(p2p)   
endif()


## INSTALL #######################

install(EXPORT sz_targets
    FILE sz_targets.cmake
    NAMESPACE sz::
    DESTINATION lib/cmake/sz
)

