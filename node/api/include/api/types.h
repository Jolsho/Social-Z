#pragma once
#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t ConnID;
typedef int SSID;
typedef int TrxID;
typedef int16_t PktCode;

#define HASH_SIZE 32
struct HashT {
    unsigned char b[HASH_SIZE];
};

#define KEY_SIZE 32
struct Key {
    unsigned char b[KEY_SIZE];
};

#define SIGNATURE_SIZE 32
struct Signature {
    unsigned char b[SIGNATURE_SIZE];
};

#define NONCE_SIZE 8
struct Nonce {
    unsigned char b[NONCE_SIZE];
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
