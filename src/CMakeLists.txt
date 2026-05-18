set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "*.cpp")

# Remove main.cpp from library sources
#list(FILTER CORE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

add_library(sz_lib ${CORE_SOURCES})

target_include_directories(sz_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${LIBS}/include
)

# OPEN SSL
set(SSL_DIR ${LIBS}/openssl)
target_include_directories(sz_lib PRIVATE ${SSL_DIR}/include)
target_link_libraries(sz_lib PRIVATE
    ${SSL_DIR}/lib/libssl.a
    ${SSL_DIR}/lib/libcrypto.a
)

# SODIUM
set(SODIUM_DIR ${LIBS}/libsodium)
target_include_directories(sz_lib PUBLIC ${SODIUM_DIR}/include)
target_link_libraries(sz_lib PUBLIC ${SODIUM_DIR}/lib/libsodium.a)


# LMDB, BLAKE3, llhttp
target_link_libraries(sz_lib PRIVATE
    ${LIBS}/lmdb/liblmdb.a
    ${LIBS}/blake3/libblake3.a
    ${LIBS}/llhttp/libllhttp.a
)


# SQLITE, simdjson
target_sources(sz_lib PRIVATE
    ${LIBS}/sqlite/sqlite3.c
    ${LIBS}/simdjson/simdjson.cpp
)

