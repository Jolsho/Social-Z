set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(log ${CORE_SOURCES})
add_library(sz::log ALIAS log)

target_include_directories(log
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/socialz/log>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(log PRIVATE sz::api sz::utils)

set_target_properties(log PROPERTIES
    OUTPUT_NAME "sz_log"
    OUTPUT_NAME_DEBUG "sz_log"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/socialz
)
install(TARGETS log
    EXPORT sz_targets
    ARCHIVE DESTINATION lib
)
