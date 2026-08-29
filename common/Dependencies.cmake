include(FetchContent)
include(ExternalProject)

set(DEPS_SRC_DIR ${CMAKE_BINARY_DIR}/_deps)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

##################################
## SODIUM
##################################

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "wasm32")
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


##################################
## BLAKE3
##################################

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
