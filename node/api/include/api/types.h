#pragma once
#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t ConnID;
typedef int SSID;
typedef int TrxID;

#define HASH_SIZE 32
struct HashT {
    unsigned char b[HASH_SIZE];
};

struct Vec {
    unsigned char*  b;
    unsigned char*  c;
    size_t          len;
    size_t          cap;
};

struct Card {
    int     id;
    HashT   hash;
    int     created_at;
};

#ifdef __cplusplus
}
#endif
