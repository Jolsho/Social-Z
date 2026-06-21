#pragma once
#include "types.h"
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t DB_PATH; 
#define DB_USER_INSERT      0
#define DB_USER_DELETE      1
#define DB_CARD_RECENT      2
#define DB_PERM_INSERT      3
#define DB_PERM_DELETE      4
#define DB_VOUCHER_INSERT   5
#define DB_NEW_BLOB         6

struct Card {
    int     id;
    HashT   hash;
    int     created_at;
};


#ifdef __cplusplus
}
#endif
