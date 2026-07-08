file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_library(p2p OBJECT ${CORE_SOURCES})
target_include_directories(p2p PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(p2p PRIVATE core)
install(FILES ${CMAKE_SOURCE_DIR}/include/sz/p2p.h DESTINATION include/sz/)

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
#     add_executable(p2p_tests ${SOURCES})
#
#     target_include_directories(p2p_tests
#         PRIVATE
#             ${CMAKE_SOURCE_DIR}/src
#     )
#
#     target_link_libraries(p2p_tests
#         PRIVATE p2p
#         PRIVATE GTest::gtest_main
#     )
#
#     include(GoogleTest)
#     gtest_discover_tests(p2p_tests)
# endif()
