set(store_sources
    ../src/utils/store.c ../src/utils/buffers.c
    ../../common/src/structs/hashtable.c ../../common/src/structs/pqueue.c)

sz_add_test(client store_priority store_priority.c ${store_sources})
sz_add_test(client store_eviction store_eviction.c ${store_sources})
sz_add_test(client context context.c
    ../src/netwrk/context.c ../src/netwrk/client.c ../src/utils/buffers.c)

if(UNIX)
    target_link_libraries(sz_store_priority_test PRIVATE m)
    target_link_libraries(sz_store_eviction_test PRIVATE m)
endif()

# feed_init is defined in a header, so compile the context test as one unit.
set_target_properties(sz_context_test PROPERTIES UNITY_BUILD ON)
set_tests_properties(client.store_eviction PROPERTIES TIMEOUT 10)
