set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(log ${CORE_SOURCES})
add_library(sz::log ALIAS log)

target_include_directories(log
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/log>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(log PRIVATE sz sz::utils)

set_target_properties(log PROPERTIES
    OUTPUT_NAME "szlog"
    OUTPUT_NAME_DEBUG "szlog"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/log
)

install(TARGETS log
    EXPORT sz_targets
    ARCHIVE DESTINATION lib
)
