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
