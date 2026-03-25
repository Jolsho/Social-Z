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

####################################################
#                   SODIUM
####################################################
set(SODIUM_DIR ${CMAKE_SOURCE_DIR}/lib/libsodium)
target_include_directories(sz PRIVATE ${SODIUM_DIR}/include)
target_link_libraries(sz PRIVATE ${SODIUM_DIR}/lib/libsodium.a)


####################################################
#                   LMDB
####################################################
set(LMDB_DIR ${CMAKE_SOURCE_DIR}/lib/lmdb)
target_include_directories(sz PRIVATE ${LMDB_DIR}/include)
target_link_libraries(sz PRIVATE ${LMDB_DIR}/lib/liblmdb.a)


####################################################
#                   BLAKE3
####################################################
set(BLAKE3_DIR ${CMAKE_SOURCE_DIR}/lib/blake3)
target_include_directories(sz PRIVATE ${BLAKE3_DIR}/include)
target_link_libraries(sz PRIVATE ${BLAKE3_DIR}/lib/libblake3.a)
