#pragma once
#include <cstring>
typedef struct SZT SZT;

typedef int SSID;
typedef int TrxID;


struct HashT {
    unsigned char b[32] {0};
};

#define HASH_SIZE 32

struct Card {
    int     id;
    HashT   hash;
    int     created_at;
};
