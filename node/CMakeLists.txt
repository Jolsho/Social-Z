## COMPONENTS ##################################

option(EXECUTABLE   "Default Build"     ON)
option(DB           "Add DB Actor"      OFF)
option(FS           "Add FS Actor"      OFF)
option(LOG          "Add LOG Actor"     OFF)
option(P2P          "Add P2P Actor"     OFF)
option(LEDGER       "Add Ledger Actor"  OFF)
option(PROXY        "Add PROXY Actor"   OFF)


if(EXECUTABLE)
    set(DB          ON CACHE BOOL "" FORCE)
    set(FS          ON CACHE BOOL "" FORCE)
    set(LOG         ON CACHE BOOL "" FORCE)
    set(P2P         ON CACHE BOOL "" FORCE)
    set(LEDGER      ON CACHE BOOL "" FORCE)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/Dependencies.cmake)


## CORE ##################################

file(GLOB API_SRCS CONFIGURE_DEPENDS "api/*.c")
file(GLOB UTIL_SRCS CONFIGURE_DEPENDS "utils/src/*.c")

add_library(node STATIC ${API_SRCS} ${UTIL_SRCS})

set_target_properties(node PROPERTIES OUTPUT_NAME "sz_node")

target_include_directories(node
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/utils/include
)

target_link_libraries(node PRIVATE common liblmdb sqlite blst)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/sz_node/ DESTINATION include/sz_node)


## DATA BASE ################
if(DB)
    file(GLOB_RECURSE DB_SOURCES CONFIGURE_DEPENDS "db/src/*.cpp")
    target_sources(node PRIVATE ${DB_SOURCES})
    target_include_directories(node 
        PUBLIC 
            ${CMAKE_CURRENT_SOURCE_DIR}/db/include

        PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/db/src
    )
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/db/include/sz_node/db.h DESTINATION include/sz_node)
endif()


## FILE SYSTEM ################
if(FS)
    file(GLOB_RECURSE FS_SOURCES CONFIGURE_DEPENDS "fs/src/*.cpp")
    target_sources(node PRIVATE ${FS_SOURCES})
    target_include_directories(node 
        PUBLIC 
            ${CMAKE_CURRENT_SOURCE_DIR}/fs/include

        PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/fs/src
    )
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/fs/include/sz_node/fs.h DESTINATION include/sz_node/)
endif()


## Peer 2 Peer Networker ################
if(P2P)
    file(GLOB_RECURSE P2P_SOURCES CONFIGURE_DEPENDS "p2p/src/*.cpp")
    target_sources(node PRIVATE ${P2P_SOURCES})
    target_include_directories(node 
        PUBLIC 
            ${CMAKE_CURRENT_SOURCE_DIR}/p2p/include

        PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/p2p/src
    )
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/p2p/include/sz_node/p2p.h DESTINATION include/sz_node/)
endif()


## LOGGER ################
if(LOG)
    file(GLOB_RECURSE LOG_SOURCES CONFIGURE_DEPENDS "log/src/*.cpp")
    target_sources(node PRIVATE ${LOG_SOURCES})
    target_include_directories(node 
        PUBLIC 
            ${CMAKE_CURRENT_SOURCE_DIR}/log/include

        PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/log/src
    )
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/log/include/sz_node/log.h DESTINATION include/sz_node/)
endif()


install(TARGETS node
    ARCHIVE DESTINATION lib
)


if(EXECUTABLE)
    add_executable(sz_exec main.cpp)

    target_link_libraries(sz_exec PRIVATE common node)

    set_target_properties(sz_exec PROPERTIES
        OUTPUT_NAME "socialz"
        OUTPUT_NAME_DEBUG "socialz"
    )
    install(TARGETS sz_exec RUNTIME DESTINATION bin)
endif()


configure_file(
    sz.pc.in
    ${CMAKE_BINARY_DIR}/sz.pc
    @ONLY
)

install(
    FILES ${CMAKE_BINARY_DIR}/sz.pc
    DESTINATION lib/pkgconfig
)


# TESTING
# option(NODE_BUILD_TESTS "Build node tests" ON)
# if(NODE_BUILD_TESTS)
#     enable_testing()
#     include(FetchContent)
#
#     FetchContent_Declare(
#       googletest
#       GIT_REPOSITORY https://github.com/google/googletest.git
#       GIT_TAG v1.14.0
#     )
#
#     FetchContent_MakeAvailable(googletest)
#
#     file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
#
#     add_executable(p2p_tests ${SOURCES})
#
#     target_include_directories(p2p_tests
#         PRIVATE
#             ${CMAKE_SOURCE_DIR}/src
#     )
#
#     target_link_libraries(p2p_tests
#         PRIVATE p2p
#         PRIVATE GTest::gtest_main
#     )
#
#     include(GoogleTest)
#     gtest_discover_tests(p2p_tests)
# endif()
