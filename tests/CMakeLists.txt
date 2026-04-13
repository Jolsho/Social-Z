include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(sz_tests ${SOURCES})

target_link_libraries(sz_tests
    PRIVATE sz_lib
    PRIVATE GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(sz_tests)
