#include "sz/client.h"
#include "sz/crypto.h"
#include <stdlib.h>

CryptCtx* new_ctx(UserCtx* user, const uint8_t* head, size_t head_len) {
    CryptCtx* ctx = malloc(sizeof(CryptCtx));
    return ctx;
}

bool decrypt(CryptCtx* ctx, const uint8_t* buf, size_t len) {
    int s = encoded_key_len();
    return true;
}

bool encrypt(CryptCtx* ctx, const uint8_t* buf, size_t len) {
    return true;
}

Iterator* iterator(const uint8_t* buf, size_t len, int obj_enum) {
    return NULL;
}
size_t size(Iterator* it) {
    return 0;
}
size_t remaining(Iterator* it) {
    return 0;
}

Card* seek_card(Iterator* it, size_t idx) {
    return NULL;
}
Card* next_card(Iterator* it) {
    return NULL; 
}
Card* prev_card(Iterator* it) {
    return NULL;
}
