file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "*.cpp")
add_library(sz_api ${CORE_SOURCES})
target_include_directories(sz_api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sz_api PRIVATE sz_utils)

