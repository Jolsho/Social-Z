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

include(${CMAKE_CURRENT_LIST_DIR}/Blake3.cmake)

install(
    TARGETS blake3
    ARCHIVE DESTINATION lib
)
install(FILES 
    ${blake3_SOURCE_DIR}/c/blake3.h
    DESTINATION include/
)
