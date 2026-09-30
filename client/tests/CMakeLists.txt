add_executable(sz_store_priority_test
    store_priority.c
    ../src/utils/store.c
    ../src/utils/buffers.c
    ../../common/src/structs/hashtable.c
    ../../common/src/structs/pqueue.c)
target_include_directories(sz_store_priority_test PRIVATE
    ../src ../../common/include)
set_target_properties(sz_store_priority_test PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_link_libraries(sz_store_priority_test PRIVATE sz_test_options)
if(UNIX)
    target_link_libraries(sz_store_priority_test PRIVATE m)
endif()

add_test(NAME client.store_priority COMMAND sz_store_priority_test)
set_tests_properties(client.store_priority PROPERTIES LABELS client)
