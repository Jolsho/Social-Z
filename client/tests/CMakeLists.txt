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

add_executable(sz_context_test context.c
    ../src/netwrk/context.c ../src/netwrk/client.c ../src/utils/buffers.c)
target_include_directories(sz_context_test PRIVATE
    ../src ../include ../../common/include)
# client.h includes feed.h, which currently defines feed_init in the header.
# A single translation unit avoids changing that unrelated linkage here.
set_target_properties(sz_context_test PROPERTIES
    UNITY_BUILD ON C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_link_libraries(sz_context_test PRIVATE sz_test_options)
add_test(NAME client.context COMMAND sz_context_test)
set_tests_properties(client.context PROPERTIES LABELS client)
