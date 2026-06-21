file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(sz_utils ${CORE_SOURCES})

target_include_directories(sz_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sz_utils PRIVATE sz_api)

if(USE_SODIUM)
    target_sources(sz_utils PRIVATE src/crypto/keys.cpp)
    target_include_directories(sz_utils 
        PUBLIC 
            src/crypto
            ${DEPS_INSTALL_DIR}/include
    )
    target_link_libraries(sz_utils PUBLIC $<$<TARGET_EXISTS:sodium>:sodium>)
endif()

if(USE_LMDB)
    target_sources(sz_utils PRIVATE src/lmdb/db.cpp)
    target_include_directories(sz_utils PUBLIC src/lmdb)
    target_link_libraries(sz_utils PUBLIC $<$<TARGET_EXISTS:lmdb>:lmdb>)
endif()

if(USE_BLAKE3)
    target_include_directories(sz_utils PUBLIC src/hash)
    target_link_libraries(sz_utils PUBLIC $<$<TARGET_EXISTS:blake3>:blake3>)
endif()

