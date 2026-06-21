set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(sz_db ${CORE_SOURCES})
target_include_directories(sz_db
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(sz_db PRIVATE 
    sz_api sz_utils
    $<$<TARGET_EXISTS:sqlite>:sqlite>
    $<$<TARGET_EXISTS:simdjson>:simdjson>
)
