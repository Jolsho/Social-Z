# Options shared by project test executables.
add_library(sz_test_options INTERFACE)

# Tests use assertions, including when built with a release configuration.
if(MSVC)
    target_compile_options(sz_test_options INTERFACE /UNDEBUG)
else()
    target_compile_options(sz_test_options INTERFACE -UNDEBUG)
endif()

if(SZ_TEST_SANITIZERS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        message(FATAL_ERROR "SZ_TEST_SANITIZERS requires GCC or Clang")
    endif()
    target_compile_options(sz_test_options INTERFACE
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(sz_test_options INTERFACE -fsanitize=address,undefined)
endif()

function(sz_add_test component name)
    set(target sz_${name}_test)
    add_executable(${target} ${ARGN})
    target_include_directories(${target} PRIVATE
        ${PROJECT_SOURCE_DIR}/${component}/src
        ${PROJECT_SOURCE_DIR}/${component}/include
        ${PROJECT_SOURCE_DIR}/common/include)
    set_target_properties(${target} PROPERTIES
        C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
    target_link_libraries(${target} PRIVATE sz_test_options)
    add_test(NAME ${component}.${name} COMMAND ${target})
    set_tests_properties(${component}.${name} PROPERTIES LABELS ${component})
endfunction()

# Components register their tests explicitly in their own CMakeLists.txt.
foreach(component IN ITEMS common client node)
    if(EXISTS "${PROJECT_SOURCE_DIR}/${component}/tests/CMakeLists.txt")
        add_subdirectory("${PROJECT_SOURCE_DIR}/${component}/tests"
                         "${PROJECT_BINARY_DIR}/tests/${component}")
    endif()
endforeach()
