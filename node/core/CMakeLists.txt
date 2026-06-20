
set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/core/*.cpp")

add_library(sz_core ${CORE_SOURCES})

target_include_directories(sz_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)


if(CRYPTO)
    target_sources(sz_core PRIVATE src/crypto/keys.cpp)
    set(SODIUM_DIR ${LIBS}/libsodium)
    target_include_directories(sz_core PUBLIC 
        src/crypto
        ${SODIUM_DIR}/include
    )
    target_link_libraries(sz_core PUBLIC ${SODIUM_DIR}/lib/libsodium.a)
endif()

if(LMDB)
    target_sources(sz_core PRIVATE src/lmdb/db.cpp)
    target_include_directories(sz_core PUBLIC 
        src/lmdb
        ${LIBS}/lmdb
    )
    target_link_libraries(sz_core PUBLIC ${LIBS}/lmdb/liblmdb.a)
endif()

if(HASH)
    target_include_directories(sz_core PUBLIC 
        src/hash
        ${LIBS}/blake3
    )
    target_link_libraries(sz_core PUBLIC ${LIBS}/blake3/libblak3.a)
endif()

