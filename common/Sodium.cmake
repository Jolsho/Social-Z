include(ExternalProject)

if(TARGET sodium)
    return()
endif()

set(SODIUM_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
if(SZ_TESTS_ONLY)
    set(SODIUM_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/_deps/sodium/install")
endif()

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "wasm32")
    set(SODIUM_CONFIGURE_CMD emconfigure ./configure --prefix=${SODIUM_INSTALL_PREFIX} --disable-shared --without-pthreads --disable-ssp --disable-asm --disable-pie && emmake make clean)
    set(SODIUM_BUILD_CMD emmake make -j2 install)
    set(SODIUM_INSTALL_CMD "")
else()
    set(SODIUM_CONFIGURE_CMD ./configure --prefix=${SODIUM_INSTALL_PREFIX} --disable-shared)
    set(SODIUM_BUILD_CMD make -j)
    set(SODIUM_INSTALL_CMD make install)
endif()

ExternalProject_Add(sodium_ep
    PREFIX ${CMAKE_BINARY_DIR}/_deps/sodium
    URL https://download.libsodium.org/libsodium/releases/libsodium-1.0.21-stable.tar.gz
    CONFIGURE_COMMAND ${SODIUM_CONFIGURE_CMD}
    BUILD_COMMAND ${SODIUM_BUILD_CMD}
    INSTALL_COMMAND ${SODIUM_INSTALL_CMD}
    BUILD_BYPRODUCTS "${SODIUM_INSTALL_PREFIX}/lib/libsodium.a"
    BUILD_IN_SOURCE 1
)

add_library(sodium STATIC IMPORTED GLOBAL)
add_dependencies(sodium sodium_ep)

file(MAKE_DIRECTORY "${SODIUM_INSTALL_PREFIX}/include")

set_target_properties(sodium PROPERTIES
    IMPORTED_LOCATION "${SODIUM_INSTALL_PREFIX}/lib/libsodium.a"
    INTERFACE_INCLUDE_DIRECTORIES "${SODIUM_INSTALL_PREFIX}/include"
)

