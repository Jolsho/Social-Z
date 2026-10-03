sz_add_test(common pqueue pqueue.c ../src/structs/pqueue.c)
sz_add_test(common hashtable_reserve hashtable_reserve.c ../src/structs/hashtable.c)
sz_add_test(common hashtable_storage hashtable_storage.c ../src/structs/hashtable.c)
sz_add_test(common requests requests.c ../src/requests/requests.c)

include(../Blake3.cmake)
sz_add_test(common hash hash.c ../src/hash/hash.c)
target_link_libraries(sz_hash_test PRIVATE blake3)

include(../Sodium.cmake)
sz_add_test(common crypto crypto.c ../src/crypto/crypto.c)
target_link_libraries(sz_crypto_test PRIVATE sodium)

sz_add_test(common owner owner.c ../src/codec/owner.c ../src/crypto/crypto.c ../src/hash/hash.c)
target_link_libraries(sz_owner_test PRIVATE sodium blake3)

sz_add_test(common blob_requests blob_requests.c ../src/requests/requests.c
    ../src/requests/blob.c ../src/crypto/crypto.c ../src/hash/hash.c)
target_link_libraries(sz_blob_requests_test PRIVATE sodium blake3)

sz_add_test(common hash_batches hash_batches.c ../src/requests/requests.c
    ../src/requests/hash_batch.c ../src/crypto/crypto.c ../src/hash/hash.c)
target_link_libraries(sz_hash_batches_test PRIVATE sodium blake3)
