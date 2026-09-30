include(FetchContent)

if(TARGET blake3)
    return()
endif()

FetchContent_Declare(blake3
    GIT_REPOSITORY https://github.com/BLAKE3-team/BLAKE3.git
    GIT_TAG 1.8.5
    SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/blake3/src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/blake3/build"
    SOURCE_SUBDIR c)
FetchContent_MakeAvailable(blake3)
