#include "utils/crypto.h"
#include "sodium/crypto_box.h"
#include "sodium/utils.h"
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sodium/crypto_secretstream_xchacha20poly1305.h>

int new_keypair(KeyPair& keys) {
    return crypto_box_keypair(keys.pub.data(), keys.priv.data());
}

std::string key_to_str(Key key) {
    size_t b64_len = sodium_base64_encoded_len(key.size(), sodium_base64_VARIANT_ORIGINAL);
    char key_str[b64_len];
    sodium_bin2base64(
        key_str,
        b64_len,
        key.data(),
        key.size(),
        sodium_base64_VARIANT_ORIGINAL
    );
    return key_str;
}

int str_to_key(const char* key_str, Key& key) {
    size_t b64_len = sodium_base64_encoded_len(key.size(), sodium_base64_VARIANT_ORIGINAL);
    size_t actual_len = 0;
    int r = sodium_base642bin(
        key.data(),     // output buffer
        key.size(),     // max output length
        key_str,        // input string
        b64_len,        // length of input string
        nullptr,        // ignore chars
        &actual_len,    // actual bytes written
        nullptr,        // end pointer (optional)
        sodium_base64_VARIANT_ORIGINAL
    );
    if (r < 0) return r;
    return b64_len;
}

