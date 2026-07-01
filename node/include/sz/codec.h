#pragma once
#include <time.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t ConnID;
typedef int SSID;
typedef int TrxID;
typedef int16_t PktCode;

#define HASH_SIZE 32
typedef struct HashT {
    unsigned char b[HASH_SIZE];
} HashT;

#define KEY_SIZE 32
typedef struct Key {
    unsigned char b[KEY_SIZE];
} Key;

typedef struct KeyPair {
    Key priv;
    Key pub;
} KeyPair;

#define SIGNATURE_SIZE 64
typedef struct Signature {
    unsigned char b[SIGNATURE_SIZE];
} Signature;

#define NONCE_SIZE 8
typedef struct Nonce {
    unsigned char b[NONCE_SIZE];
} Nonce;

typedef struct Card {
    int     id;
    HashT   hash;
    int     created_at;
} Card;

#define PERM_DATA_SIZE 256
static const size_t PERM_SIZE_NOPAD = (KEY_SIZE * 2) + NONCE_SIZE + PERM_DATA_SIZE + SIGNATURE_SIZE;
typedef struct Perm {
    Key         giver;
    Key         recipient;
    Nonce       nonce;
    uint8_t     data[PERM_DATA_SIZE];
    Signature   signature;
} Perm;
HashT hash_perm(Perm* p);
size_t marshal_perm(uint8_t** pb, Perm* p);
size_t unmarshal_perm(uint8_t** pb, Perm* p);


#define VOUCH_DATA_SIZE 256
static const size_t VOUCH_SIZE_NOPAD = (KEY_SIZE * 2) + HASH_SIZE + sizeof(size_t) + sizeof(time_t) + VOUCH_DATA_SIZE + SIGNATURE_SIZE;
typedef struct Voucher {
    Key         to;
    Key         from;

    HashT       file_hash;
    size_t      file_size;
    time_t      expiration;

    unsigned char   data[VOUCH_DATA_SIZE];
    Signature   signature;

} Voucher;
HashT hash_voucher(Voucher* v);
size_t marshal_voucher(uint8_t** pb, Voucher* v);
size_t unmarshal_voucher(uint8_t** pb, Voucher* v);


#ifdef __cplusplus
}
#endif
