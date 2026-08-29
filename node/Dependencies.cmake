include(FetchContent)
include(ExternalProject)

set(DEPS_SRC_DIR ${CMAKE_BINARY_DIR}/_deps)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

##################################
## LMDB
##################################
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

##################################
## SQLITE
##################################
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


##################################
## BLST
##################################
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
