#pragma once
#include "bindings.h"
#include "utils/key.h"
#include "sodium/crypto_sign.h"
#include <array>

using Signature = std::array<unsigned char, crypto_sign_BYTES>;
static constexpr size_t SIG_SIZE = sizeof(Signature);


inline bool valid_signature(Key& signer, Signature& sig, HashT& hash) {
    return crypto_sign_verify_detached(sig.data(), hash.b, HASH_SIZE, signer.data());
}

