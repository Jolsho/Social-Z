file(GLOB CODEC_SRCS CONFIGURE_DEPENDS "codec/*.c")
file(GLOB HASH_SRCS CONFIGURE_DEPENDS "hash/*.c")

add_library(core STATIC ${CODEC_SRCS} ${HASH_SRCS})
set_target_properties(core PROPERTIES OUTPUT_NAME "sz_core")

target_include_directories(core
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core PUBLIC blake3)
install(FILES 
    ${CMAKE_SOURCE_DIR}/include/sz/hash.h 
    ${CMAKE_SOURCE_DIR}/include/sz/codec.h
    DESTINATION include/sz/
)
install(TARGETS core
    EXPORT sz_targets
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

if(USE_CRYPTO)
    file(GLOB CRYPTO_SRCS CONFIGURE_DEPENDS "crypto/*.c")
    target_sources(core PRIVATE ${CRYPTO_SRCS})
    install(FILES 
        ${CMAKE_SOURCE_DIR}/include/sz/crypto.h 
        DESTINATION include/sz/
    )
    target_link_libraries(core PUBLIC sodium)
endif()

if(USE_LMDB)
    file(GLOB_RECURSE LMDB_SRCS CONFIGURE_DEPENDS "lmdb/*.c")
    target_sources(core PRIVATE ${LMDB_SRCS})
    install(FILES 
        ${CMAKE_SOURCE_DIR}/include/sz/lmdb.h 
        DESTINATION include/sz/
    )
    target_link_libraries(core PUBLIC liblmdb)
endif()

if(NOT EMSCRIPTEN)
    file(GLOB API_SRCS CONFIGURE_DEPENDS "api/*.c")
    target_sources(core PRIVATE ${API_SRCS})
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/sz/api    DESTINATION include/sz)
    
    file(GLOB UTIL_SRCS CONFIGURE_DEPENDS "utils/*.c")
    target_sources(core PRIVATE ${UTIL_SRCS})
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/sz/utils  DESTINATION include/sz)
endif()
