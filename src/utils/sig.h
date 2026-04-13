#pragma once
#include "crypto.h"
#include "sodium/crypto_sign.h"
#include <array>

using Signature = std::array<std::byte, crypto_sign_BYTES>;
static constexpr size_t SIG_SIZE = sizeof(Signature);


inline bool valid_signature(Key& signer, Signature& sig, Hash& hash) {
    return crypto_sign_verify_detached(
        reinterpret_cast<const unsigned char*>(sig.data()), 
        reinterpret_cast<const unsigned char*>(hash.data()), HASH_SIZE, 
        reinterpret_cast<const unsigned char*>(signer.data())
    );
}

