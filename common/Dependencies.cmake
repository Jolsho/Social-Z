set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

include(${CMAKE_CURRENT_LIST_DIR}/Sodium.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/Blake3.cmake)

install(
    TARGETS blake3
    ARCHIVE DESTINATION lib
)
install(FILES 
    ${blake3_SOURCE_DIR}/c/blake3.h
    DESTINATION include/
)
