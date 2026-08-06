file(GLOB_RECURSE CLIENT_SRCS CONFIGURE_DEPENDS "*.c")

add_library(client STATIC ${CLIENT_SRCS})

target_include_directories(client
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(NOT EMSCRIPTEN)
    target_compile_definitions(client PRIVATE NATIVE)
endif()

target_link_libraries(client PRIVATE core)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/sz/client DESTINATION include/sz/)
set_target_properties(client PROPERTIES OUTPUT_NAME "sz_client")
install(TARGETS client ARCHIVE DESTINATION lib)
