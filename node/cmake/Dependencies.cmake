include(FetchContent)
include(ExternalProject)

set(DEPS_SRC_DIR ${CMAKE_BINARY_DIR}/_deps)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)


##################################
## Dependency Options
##################################

option(USE_BLAKE3   "Enable BLAKE3"     ON)
option(USE_CRYPTO   "Enable libsodium"  OFF)
option(USE_LMDB     "Enable LMDB"       OFF)
option(USE_SQLITE   "Enable SQLite"     OFF)
option(USE_BLST     "Enable libblst"    OFF)

##################################
## Dependency logic coupling
##################################
if (CLIENT) 
    set(USE_BLAKE3 ON CACHE BOOL "" FORCE)
    set(USE_CRYPTO ON CACHE BOOL "" FORCE)
endif()

if (LEDGER)
    set(USE_BLST   ON CACHE BOOL "" FORCE)
    set(USE_LMDB   ON CACHE BOOL "" FORCE)
    set(USE_BLAKE3 ON CACHE BOOL "" FORCE)
endif()

if (FS)
    set(USE_UTILS  ON CACHE BOOL "" FORCE)
    set(USE_LMDB   ON CACHE BOOL "" FORCE)
    set(USE_CRYPTO ON CACHE BOOL "" FORCE)
    set(USE_BLAKE3 ON CACHE BOOL "" FORCE)
endif()

if (DB)
    set(USE_UTILS    ON CACHE BOOL "" FORCE)
    set(USE_LMDB     ON CACHE BOOL "" FORCE)
    set(USE_SQLITE   ON CACHE BOOL "" FORCE)
endif()

if (P2P)
    set(USE_UTILS  ON CACHE BOOL "" FORCE)
    set(USE_CRYPTO ON CACHE BOOL "" FORCE)
endif()

if (RPC)
    set(USE_UTILS    ON CACHE BOOL "" FORCE)
endif()


##################################
## SODIUM
##################################
if(USE_CRYPTO)

    if(EMSCRIPTEN)
        set(CONFIGURE_CMD emconfigure ./configure --prefix=${CMAKE_INSTALL_PREFIX} --disable-shared --without-pthreads --disable-ssp --disable-asm --disable-pie && emmake make clean)
        set(BUILD_CMD emmake make -j2 install)
        set(INSTALL_CMD "")
    else()
        set(CONFIGURE_CMD ./configure --prefix=${CMAKE_INSTALL_PREFIX} --disable-shared)
        set(BUILD_CMD make -j)
        set(INSTALL_CMD make install)
    endif()

    ExternalProject_Add(sodium_ep
        PREFIX ${DEPS_SRC_DIR}/sodium
        URL https://download.libsodium.org/libsodium/releases/libsodium-1.0.21-stable.tar.gz
        CONFIGURE_COMMAND ${CONFIGURE_CMD}
        BUILD_COMMAND ${BUILD_CMD}
        INSTALL_COMMAND ${INSTALL_CMD}
        BUILD_IN_SOURCE 1
    )

    add_library(sodium STATIC IMPORTED GLOBAL)
    add_dependencies(sodium sodium_ep)

    file(MAKE_DIRECTORY "${CMAKE_INSTALL_PREFIX}/include")

    set_target_properties(sodium PROPERTIES
        IMPORTED_LOCATION "${CMAKE_INSTALL_PREFIX}/lib/libsodium.a"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/include"
    )
endif()

##################################
## LMDB
##################################
if(USE_LMDB)
    ExternalProject_Add(lmdb_ep
        PREFIX ${DEPS_SRC_DIR}/lmdb
        GIT_REPOSITORY https://github.com/LMDB/lmdb.git
        GIT_TAG mdb.master3
        UPDATE_DISCONNECTED TRUE
        SOURCE_SUBDIR libraries/liblmdb
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make -j
        INSTALL_COMMAND ""
    )

    set(LMDB_SRC ${DEPS_SRC_DIR}/lmdb/src/lmdb_ep/libraries/liblmdb)
    file(MAKE_DIRECTORY ${LMDB_SRC})

    add_library(liblmdb STATIC IMPORTED GLOBAL)
    add_dependencies(liblmdb lmdb_ep)

    set_target_properties(liblmdb PROPERTIES
        IMPORTED_LOCATION "${LMDB_SRC}/liblmdb.a"
        INTERFACE_INCLUDE_DIRECTORIES "${LMDB_SRC}"
    )
    install(
        FILES "${LMDB_SRC}/liblmdb.a"
        DESTINATION lib
    )
    install(FILES 
        ${LMDB_SRC}/lmdb.h 
        DESTINATION include/
    )
endif()

##################################
## SQLITE
##################################
if(USE_SQLITE)

    set(SQLITE_DIR ${DEPS_SRC_DIR}/sqlite)
    set(FETCHCONTENT_BASE_DIR "${SQLITE_DIR}")


    FetchContent_Declare(
        sqlite
        URL https://www.sqlite.org/2026/sqlite-amalgamation-3510300.zip
        SOURCE_DIR "${SQLITE_DIR}/src"
        BINARY_DIR "${SQLITE_DIR}/build"
    )
    FetchContent_MakeAvailable(sqlite)

    add_library(sqlite STATIC)
    set_target_properties(sqlite PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${sqlite_BINARY_DIR}"
    )
    target_sources(sqlite PRIVATE "${sqlite_SOURCE_DIR}/sqlite3.c")
    target_include_directories(sqlite 
        PUBLIC 
            $<BUILD_INTERFACE:${sqlite_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    install(
        TARGETS sqlite
        ARCHIVE DESTINATION lib
    )
    install(FILES 
        ${sqlite_SOURCE_DIR}/sqlite3.h 
        ${sqlite_SOURCE_DIR}/sqlite3ext.h 
        DESTINATION include/
    )

endif()

##################################
## BLAKE3
##################################
if(USE_BLAKE3)
    set(BLAKE_DIR ${DEPS_SRC_DIR}/blake3)
    set(FETCHCONTENT_BASE_DIR "${BLAKE_DIR}")

    FetchContent_Declare(blake3
        GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3.git
        GIT_TAG 1.8.5

        SOURCE_DIR "${BLAKE_DIR}/src"
        BINARY_DIR "${BLAKE_DIR}/build"
    )

    FetchContent_GetProperties(blake3)

    if(NOT blake3_POPULATED)
        FetchContent_Populate(blake3)
        add_subdirectory(${blake3_SOURCE_DIR}/c ${blake3_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()

    install(
        TARGETS blake3
        ARCHIVE DESTINATION lib
    )
    install(FILES 
        ${BLAKE_DIR}/src/c/blake3.h 
        DESTINATION include/
    )

endif()


##################################
## BLST
##################################
if(USE_BLST)
    ExternalProject_Add(blst_ep
        PREFIX ${DEPS_SRC_DIR}/blst
        GIT_REPOSITORY https://github.com/supranational/blst.git
        GIT_TAG v0.3.15
        UPDATE_DISCONNECTED TRUE
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ./build.sh
        INSTALL_COMMAND ""
    )

    set(BLST_SRC ${DEPS_SRC_DIR}/blst/src/blst_ep)

    add_library(blst STATIC IMPORTED GLOBAL)
    add_dependencies(blst blst_ep)

    file(MAKE_DIRECTORY "${BLST_SRC}/bindings")

    set_target_properties(blst PROPERTIES
        IMPORTED_LOCATION ${BLST_SRC}/libblst.a
        INTERFACE_INCLUDE_DIRECTORIES ${BLST_SRC}/bindings
    )

    install(
        FILES "${BLST_SRC}/libblst.a"
        DESTINATION lib
    )
    install(FILES 
        ${BLST_SRC}/bindings/blst.h 
        DESTINATION include/
    )


endif()
