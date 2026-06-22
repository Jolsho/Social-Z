set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(p2p ${CORE_SOURCES})
add_library(sz::p2p ALIAS p2p)

target_include_directories(p2p
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/p2p>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(p2p PRIVATE 
    sz sz::utils
    $<$<TARGET_EXISTS:sodium>:sodium>
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/p2p
)

install(TARGETS p2p
    EXPORT sz_targets
    ARCHIVE DESTINATION lib
)

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
#             ${CMAKE_CURRENT_SOURCE_DIR}/src
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
