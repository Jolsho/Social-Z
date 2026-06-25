set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(db ${CORE_SOURCES})
add_library(sz::db ALIAS db)
target_include_directories(db
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/socialz/db>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(db PRIVATE 
    sz::api 
    sz::utils 
    lmdb
    $<$<TARGET_EXISTS:sqlite>:sqlite>
    $<$<TARGET_EXISTS:simdjson>:simdjson>
)

set_target_properties(db PROPERTIES
    OUTPUT_NAME "sz_db"
    OUTPUT_NAME_DEBUG "sz_db"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/socialz
)
install(TARGETS db
    EXPORT sz_targets
    ARCHIVE DESTINATION lib
)
