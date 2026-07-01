#include "sz/codec.h"
#include "sz/hash.h"
#include <string.h>

size_t marshal_perm(uint8_t** pb, Perm* p) {
    uint8_t* b = *pb;
    memcpy(b, p->giver.b, KEY_SIZE);            b += KEY_SIZE;
    memcpy(b, p->recipient.b, KEY_SIZE);        b += KEY_SIZE;
    memcpy(b, p->nonce.b, NONCE_SIZE);          b += NONCE_SIZE;
    memcpy(b, p->data, PERM_DATA_SIZE);         b += PERM_DATA_SIZE;
    memcpy(b, p->signature.b, SIGNATURE_SIZE);  b += SIGNATURE_SIZE;
    return PERM_SIZE_NOPAD;
}

size_t unmarshal_perm(uint8_t** pb, Perm* p) {
    uint8_t* b = *pb;
    memcpy(p->giver.b, b, KEY_SIZE);            b += KEY_SIZE;
    memcpy(p->recipient.b, b, KEY_SIZE);        b += KEY_SIZE;
    memcpy(p->nonce.b, b, NONCE_SIZE);          b += NONCE_SIZE;
    memcpy(p->data, b, PERM_DATA_SIZE);         b += PERM_DATA_SIZE;
    memcpy(p->signature.b, b, SIGNATURE_SIZE);  b += SIGNATURE_SIZE;
    return PERM_SIZE_NOPAD;
}

HashT hash_perm(Perm* p) {
    Hasher h = new_hasher();
    hash_update(&h, p->giver.b, KEY_SIZE);
    hash_update(&h, p->recipient.b, KEY_SIZE);
    hash_update(&h, p->nonce.b, NONCE_SIZE);
    hash_update(&h, p->data, PERM_DATA_SIZE);
    return hash_finalize(&h);
}

size_t marshal_voucher(uint8_t** pb, Voucher* v) {
    uint8_t* b = *pb;
    memcpy(b, v->from.b, KEY_SIZE);             b += KEY_SIZE;
    memcpy(b, v->file_hash.b, HASH_SIZE);       b += HASH_SIZE;
    memcpy(b, &v->file_size, sizeof(size_t));   b += sizeof(size_t);
    memcpy(b, &v->expiration, sizeof(time_t));  b += sizeof(time_t);
    memcpy(b, v->data, VOUCH_DATA_SIZE);        b += VOUCH_DATA_SIZE;
    memcpy(b, v->signature.b, SIGNATURE_SIZE);  b += SIGNATURE_SIZE;
    return VOUCH_SIZE_NOPAD;
}
size_t unmarshal_voucher(uint8_t** pb, Voucher* v) {
    uint8_t* b = *pb;
    memcpy(v->from.b, b, KEY_SIZE);             b += KEY_SIZE;
    memcpy(v->file_hash.b, b, HASH_SIZE);       b += HASH_SIZE;
    memcpy(&v->file_size, b, sizeof(size_t));   b += sizeof(size_t);
    memcpy(&v->expiration, b, sizeof(time_t));  b += sizeof(time_t);
    memcpy(v->data, b, VOUCH_DATA_SIZE);        b += VOUCH_DATA_SIZE;
    memcpy(v->signature.b, b, SIGNATURE_SIZE);  b += SIGNATURE_SIZE;
    return VOUCH_SIZE_NOPAD;
}

HashT hash_voucher(Voucher* v) {
    Hasher h = new_hasher();
    hash_update(&h, v->to.b, KEY_SIZE);
    hash_update(&h, v->from.b, KEY_SIZE);
    hash_update(&h, v->file_hash.b, HASH_SIZE);

    hash_update(&h, (uint8_t*)(&v->expiration), sizeof(time_t));
    hash_update(&h, v->data, VOUCH_DATA_SIZE);
    return hash_finalize(&h);
}
