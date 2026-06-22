file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(utils ${CORE_SOURCES})
add_library(sz::utils ALIAS utils )

target_include_directories(utils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(utils PRIVATE sz)

# INSTALL
if (USE_UTILS)
    target_include_directories(utils
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include
    )

    install(TARGETS utils
        EXPORT sz_targets
        ARCHIVE DESTINATION lib
    )
endif()

if(USE_SODIUM)
    target_sources(utils PRIVATE src/crypto/crypto.cpp)
    target_link_libraries(utils PRIVATE $<$<TARGET_EXISTS:sodium>:sodium>)

    target_include_directories(utils 
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/optional/crypto>
    )
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/optional/crypto
        DESTINATION include/utils
    )
endif()

if(USE_LMDB)
    target_sources(utils PRIVATE src/lmdb/db.cpp)
    target_link_libraries(utils PRIVATE lmdb)

    target_include_directories(utils PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/optional/lmdb>)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/optional/lmdb
        DESTINATION include/utils
    )
endif()

if(USE_BLAKE3)
    target_include_directories(utils 
        PUBLIC 
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/optional/hash>
    )
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/optional/hash
        DESTINATION include/utils
    )
    target_link_libraries(utils PRIVATE $<$<TARGET_EXISTS:blake3>:blake3>)
endif()
