function(add_includes feature)
    target_include_directories(${feature}
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/${feature}
    )
endfunction()

add_library(core OBJECT)
add_includes(core)
add_library(sz::core ALIAS core)
target_include_directories(core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB API_SRCS CONFIGURE_DEPENDS "api/*.cpp")
target_sources(core PRIVATE ${API_SRCS})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/sz/api    DESTINATION include/sz)

file(GLOB UTIL_SRCS CONFIGURE_DEPENDS "utils/*.cpp")
target_sources(core PRIVATE ${UTIL_SRCS})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/sz/utils  DESTINATION include/sz)


add_library(codec STATIC)
add_library(sz::codec ALIAS codec)
file(GLOB CODEC_SRCS CONFIGURE_DEPENDS "codec/*.c")
target_sources(codec PRIVATE ${CODEC_SRCS})
add_includes(codec)
install(FILES ${CMAKE_SOURCE_DIR}/include/sz/codec.h DESTINATION include/sz/)


add_library(hash STATIC)
add_library(sz::hash ALIAS hash)
file(GLOB HASH_SRCS CONFIGURE_DEPENDS "hash/*.c")
target_sources(hash PRIVATE ${HASH_SRCS})
install(FILES ${CMAKE_SOURCE_DIR}/include/sz/hash.h DESTINATION include/sz/)
add_includes(hash)
target_link_libraries(hash PRIVATE blake3)


if(USE_CRYPTO)
    add_library(crypto STATIC)
    add_library(sz::crypto ALIAS crypto)
    file(GLOB CRYPTO_SRCS CONFIGURE_DEPENDS "crypto/*.c")
    target_sources(crypto PRIVATE ${CRYPTO_SRCS})
    add_includes(crypto)
    target_link_libraries(crypto PUBLIC sodium)
    install(FILES ${CMAKE_SOURCE_DIR}/include/sz/crypto.h DESTINATION include/sz/)
endif()


if(USE_LMDB)
    add_library(lmdb STATIC)
    add_library(sz::lmdb ALIAS lmdb)
    file(GLOB_RECURSE LMDB_SRCS CONFIGURE_DEPENDS "lmdb/*.cpp")
    target_sources(lmdb PRIVATE ${LMDB_SRCS})
    add_includes(lmdb)
    target_link_libraries(lmdb PUBLIC liblmdb)
    install(FILES ${CMAKE_SOURCE_DIR}/include/sz/lmdb.h DESTINATION include/sz/)
endif()

target_link_libraries(core
    PUBLIC
        sz::codec
        sz::hash
        $<$<TARGET_EXISTS:sz::crypto>:sz::crypto>
        $<$<TARGET_EXISTS:sz::lmdb>:sz::lmdb>
)

