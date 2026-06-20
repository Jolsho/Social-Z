set(LIBS ${CMAKE_SOURCE_DIR}/lib)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(sz_db ${CORE_SOURCES})

target_link_libraries(sz_db PRIVATE sz_core)

target_include_directories(sz_db
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LIBS}/simdjson
        ${LIBS}/sqlite
)

# SQLITE, simdjson
target_sources(sz_db PRIVATE
    ${LIBS}/sqlite/sqlite3.c
    ${LIBS}/simdjson/simdjson.cpp
)



