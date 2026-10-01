set(store_sources
    ../src/utils/store.c ../src/utils/buffers.c
    ../../common/src/structs/hashtable.c ../../common/src/structs/pqueue.c)

sz_add_test(client store_priority store_priority.c ${store_sources})
sz_add_test(client store_eviction store_eviction.c ${store_sources})
sz_add_test(client store_lifecycle store_lifecycle.c ${store_sources})
target_compile_definitions(sz_store_lifecycle_test PRIVATE realloc=store_test_realloc)
sz_add_test(client context context.c
    ../src/netwrk/context.c ../src/netwrk/client.c ${store_sources})
target_compile_definitions(sz_context_test PRIVATE calloc=context_test_calloc)
sz_add_test(client feed feed.c ../src/data/feed.c)
sz_add_test(client blobs blobs.c ../src/data/blobs.c ../src/netwrk/client.c
    ../src/netwrk/context.c ../../common/src/hash/hash.c ${store_sources})
target_link_libraries(sz_blobs_test PRIVATE blake3)

set_tests_properties(client.store_eviction PROPERTIES TIMEOUT 10)
