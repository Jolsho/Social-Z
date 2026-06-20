#pragma once
#include <cstddef>

struct DBConfig {
    size_t          msgs_cap        { 256 };
    size_t          map_size        { 10 * 1024 * 1024 };
};

