if (CMAKE_SYSTEM_PROCESSOR STREQUAL "wasm32")
    target_compile_definitions(common PRIVATE PLATFORM_WASM32)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(common PRIVATE PLATFORM_LINUX)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(common PRIVATE PLATFORM_WINDOWS)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(common PRIVATE PLATFORM_MACOS)

endif()


file(GLOB_RECURSE CLIENT_SRCS CONFIGURE_DEPENDS "src/*.c")

add_library(client STATIC ${CLIENT_SRCS})

set_target_properties(client PROPERTIES OUTPUT_NAME "sz_client")

target_include_directories(client
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(client PRIVATE common)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/sz_client DESTINATION include)

install(TARGETS client ARCHIVE DESTINATION lib)
