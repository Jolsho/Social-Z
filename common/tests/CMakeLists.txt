add_executable(sz_pqueue_test pqueue.c ../src/structs/pqueue.c)
target_include_directories(sz_pqueue_test PRIVATE ../include)
set_target_properties(sz_pqueue_test PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)
target_link_libraries(sz_pqueue_test PRIVATE sz_test_options)

add_test(NAME common.pqueue COMMAND sz_pqueue_test)
set_tests_properties(common.pqueue PROPERTIES LABELS common)
