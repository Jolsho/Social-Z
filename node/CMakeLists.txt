set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(sz_lib ${CORE_SOURCES})

target_include_directories(sz_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
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



# TESTING
option(NODE_BUILD_TESTS "Build node tests" ON)
if(NODE_BUILD_TESTS)
    enable_testing()
    include(FetchContent)

    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.14.0
    )

    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "tests/*.cpp")

    add_executable(sz_tests ${SOURCES})

    target_include_directories(sz_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(sz_tests
        PRIVATE sz_lib
        PRIVATE GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(sz_tests)
endif()
