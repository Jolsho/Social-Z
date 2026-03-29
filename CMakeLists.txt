cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(sz LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# EXECUTABLES
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

# HEADERS
target_include_directories(${PROJECT_NAME} 
    PUBLIC 
        include
    PRIVATE
        src
)

# SODIUM
set(SODIUM_DIR ${CMAKE_SOURCE_DIR}/lib/libsodium)
target_include_directories(${PROJECT_NAME} PRIVATE ${SODIUM_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SODIUM_DIR}/lib/libsodium.a)


# LMDB
set(LMDB_DIR ${CMAKE_SOURCE_DIR}/lib/lmdb)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LMDB_DIR}/liblmdb.a)


# BLAKE3
set(BLAKE3_DIR ${CMAKE_SOURCE_DIR}/lib/blake3)
target_link_libraries(${PROJECT_NAME} PRIVATE ${BLAKE3_DIR}/libblake3.a)


# llhttp
set(LLHTTP_DIR ${CMAKE_SOURCE_DIR}/lib/llhttp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LLHTTP_DIR}/libllhttp.a)


# OPEN SSL
set(SSL_DIR ${CMAKE_SOURCE_DIR}/lib/openssl)
target_include_directories(${PROJECT_NAME} PRIVATE ${SSL_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SSL_DIR}/lib64/libssl.a)
target_link_libraries(${PROJECT_NAME} PRIVATE ${SSL_DIR}/lib64/libcrypto.a)


# SQLITE
set(SQL_DIR ${CMAKE_SOURCE_DIR}/lib/sqlite)
target_sources(${PROJECT_NAME} PRIVATE ${SQL_DIR}/sqlite3.c)


# simdjson
set(JSON_DIR ${CMAKE_SOURCE_DIR}/lib/simdjson)
target_sources(${PROJECT_NAME} PRIVATE ${JSON_DIR}/simdjson.cpp)
