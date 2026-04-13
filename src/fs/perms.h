#pragma once
#include "utils/hash.h"
#include "utils/sig.h"
#include <cstring>

struct Perm {
    static constexpr size_t DATA_SZ = 256;

    Key         giver;
    Key         recipient;
    Nonce       nonce;
    std::array<std::byte, DATA_SZ>   data;
    Signature   signature;

    Hash hash() {
        Hasher h{};
        h.update(reinterpret_cast<const std::byte*>(giver.data()), KEY_SIZE);
        h.update(reinterpret_cast<const std::byte*>(recipient.data()), KEY_SIZE);
        h.update(reinterpret_cast<const std::byte*>(nonce.data()), NONCE_SIZE);
        h.update(data.data(), DATA_SZ);
        return h.finalize();
    }

    std::byte* unmarshal(std::byte* cursor) {
        memcpy(giver.data(), cursor, KEY_SIZE);
        cursor += KEY_SIZE;

        memcpy(recipient.data(), cursor, KEY_SIZE);
        cursor += KEY_SIZE;

        memcpy(nonce.data(), cursor, NONCE_SIZE);
        cursor += NONCE_SIZE;

        memcpy(data.data(), cursor, DATA_SZ);
        cursor += DATA_SZ;

        memcpy(signature.data(), cursor, SIG_SIZE);
        cursor += SIG_SIZE;

        return cursor;
    }

    std::byte* marshal(std::byte* cursor) {
        memcpy(cursor, giver.data(), KEY_SIZE);
        cursor += KEY_SIZE;

        memcpy(cursor, recipient.data(), KEY_SIZE);
        cursor += KEY_SIZE;

        memcpy(cursor, nonce.data(), NONCE_SIZE);
        cursor += NONCE_SIZE;

        memcpy(cursor, data.data(), DATA_SZ);
        cursor += DATA_SZ;

        memcpy(cursor, signature.data(), SIG_SIZE);
        cursor += SIG_SIZE;

        return cursor;
    }

};
static constexpr size_t PERM_SZ = sizeof(Perm);
