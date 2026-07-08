#include <stdatomic.h>
#include <stdbool.h>

atomic_int should;

bool should_shutdown() {
    int v = atomic_load(&should);
    return v == 1;
}

void sz_shutdown() {
    atomic_store(&should, 1);
}
