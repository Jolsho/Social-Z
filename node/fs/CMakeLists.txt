set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(fs ${CORE_SOURCES})
add_library(sz::fs ALIAS fs)

target_include_directories(fs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/socialz/fs>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(fs PRIVATE 
    sz::api sz::utils
    $<$<TARGET_EXISTS:lmdb>:lmdb>
    $<$<TARGET_EXISTS:sodium>:sodium>
    $<$<TARGET_EXISTS:blake3>:blake3>
)

set_target_properties(fs PROPERTIES
    OUTPUT_NAME "sz_fs"
    OUTPUT_NAME_DEBUG "sz_fs"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/socialz
)

install(TARGETS fs
    EXPORT sz_targets
    ARCHIVE DESTINATION lib
)
