set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp")

add_executable(sz_http ${SOURCES})

target_include_directories(sz_http
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_SOURCE_DIR}/include
        ${LIBS}/include
)
