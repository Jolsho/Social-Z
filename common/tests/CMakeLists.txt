sz_add_test(common pqueue pqueue.c ../src/structs/pqueue.c)
sz_add_test(common hashtable_reserve hashtable_reserve.c ../src/structs/hashtable.c)
sz_add_test(common hashtable_storage hashtable_storage.c ../src/structs/hashtable.c)

include(../Blake3.cmake)
sz_add_test(common hash hash.c ../src/hash/hash.c)
target_link_libraries(sz_hash_test PRIVATE blake3)

include(../Sodium.cmake)
sz_add_test(common crypto crypto.c ../src/crypto/crypto.c)
target_link_libraries(sz_crypto_test PRIVATE sodium)
