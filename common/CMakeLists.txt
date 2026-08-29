include(Dependencies.cmake)

file(GLOB_RECURSE COMMON_SRCS CONFIGURE_DEPENDS "src/*.c")

add_library(common OBJECT ${COMMON_SRCS})

target_include_directories(common
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(common PUBLIC sodium blake3)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/sz_common DESTINATION include)

# set_target_properties(common PROPERTIES OUTPUT_NAME "sz_common")
#
# install(TARGETS common ARCHIVE DESTINATION lib)
