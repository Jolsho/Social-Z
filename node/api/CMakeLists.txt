file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_library(api ${CORE_SOURCES})
target_include_directories(api 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
add_library(sz::api ALIAS api)

set_target_properties(api PROPERTIES
    OUTPUT_NAME "sz"
    OUTPUT_NAME_DEBUG "sz"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/socialz
)
install(TARGETS api
    EXPORT sz_targets
    ARCHIVE DESTINATION lib
)

