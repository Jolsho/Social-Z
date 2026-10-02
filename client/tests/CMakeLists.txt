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
sz_add_test(client feed_page_storage feed_page_storage.c ../src/content/feed_page.c)
target_link_libraries(sz_feed_page_storage_test PRIVATE sodium)
sz_add_test(client account account.c ../src/codec/account.c ../src/netwrk/login.c ../src/utils/crypto.c)
target_link_libraries(sz_account_test PRIVATE sodium)
sz_add_test(client blobs blobs.c ../src/netwrk/parsers/blob.c ../src/netwrk/client.c
    ../src/netwrk/context.c ../../common/src/hash/hash.c ${store_sources})
target_link_libraries(sz_blobs_test PRIVATE blake3)
set(client_sources ../src/client.c ../src/content/feed.c ../src/content/feed_page.c ../src/login.c
    ../src/netwrk/marshalers/user.c ../src/netwrk/parsers/user.c
    ../src/netwrk/parsers/blob.c ../src/netwrk/client.c ../src/netwrk/context.c
    ../../common/src/hash/hash.c ../src/netwrk/login.c
    ../src/codec/account.c ../src/utils/crypto.c ${store_sources})
sz_add_test(client lifecycle lifecycle.c ${client_sources})
target_link_libraries(sz_lifecycle_test PRIVATE blake3 sodium)
target_compile_definitions(sz_lifecycle_test PRIVATE
    malloc=client_test_malloc calloc=client_test_calloc)

sz_add_test(client login login.c ${client_sources})
target_link_libraries(sz_login_test PRIVATE blake3 sodium)

set_tests_properties(client.store_eviction PROPERTIES TIMEOUT 10)

sz_add_test(client feed_page feed_page.c ../src/codec/feed_page.c ../src/content/feed_page.c)
target_link_libraries(sz_feed_page_test PRIVATE sodium)
target_compile_definitions(sz_feed_page_test PRIVATE
    malloc=feed_page_test_malloc calloc=feed_page_test_calloc free=feed_page_test_free)

sz_add_test(client blob_crypto crypto.c ../src/utils/crypto.c ../src/codec/feed_page.c ../src/content/feed_page.c)
target_link_libraries(sz_blob_crypto_test PRIVATE sodium)

sz_add_test(client package package.c ../src/codec/package.c)

sz_add_test(client package_storage package_storage.c ../src/content/package.c)
target_link_libraries(sz_package_storage_test PRIVATE sodium)
target_compile_definitions(sz_package_storage_test PRIVATE
    calloc=package_test_calloc free=package_test_free)
