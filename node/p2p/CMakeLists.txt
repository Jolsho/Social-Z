set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(sz_p2p ${CORE_SOURCES})

target_include_directories(sz_p2p
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(sz_p2p PRIVATE sz_api sz_utils)

# TESTING
# option(NODE_BUILD_TESTS "Build node tests" ON)
# if(NODE_BUILD_TESTS)
#     enable_testing()
#     include(FetchContent)
#
#     FetchContent_Declare(
#       googletest
#       GIT_REPOSITORY https://github.com/google/googletest.git
#       GIT_TAG v1.14.0
#     )
#
#     FetchContent_MakeAvailable(googletest)
#
#     file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
#
#     add_executable(sz_p2p_tests ${SOURCES})
#
#     target_include_directories(sz_p2p_tests
#         PRIVATE
#             ${CMAKE_CURRENT_SOURCE_DIR}/src
#     )
#
#     target_link_libraries(sz_p2p_tests
#         PRIVATE sz_p2p
#         PRIVATE GTest::gtest_main
#     )
#
#     include(GoogleTest)
#     gtest_discover_tests(sz_p2p_tests)
# endif()
