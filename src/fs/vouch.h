#pragma once
#include "utils/hash.h"
#include "utils/sig.h"
#include <cstring>
#include <ctime>

struct Voucher {
    static constexpr size_t TIME_SIZE = sizeof(time_t);

    Key         to;
    Key         from;

    Hash        file_hash;
    time_t      expiration;

    Signature   signature;

    Hash hash() {
        Hasher h {};
        h.update(file_hash.data(), HASH_SIZE);

        auto raw = reinterpret_cast<const std::byte*>(&expiration);
        h.update(raw, TIME_SIZE);

        return h.finalize();
    }

    std::byte* unmarshal(std::byte* cursor) {
        memcpy(to.data(), cursor, KEY_SIZE);
        cursor += KEY_SIZE;

        memcpy(from.data(), cursor, KEY_SIZE);
        cursor += KEY_SIZE;

        memcpy(file_hash.data(), cursor, HASH_SIZE);
        cursor += HASH_SIZE;

        memcpy(&expiration, cursor, TIME_SIZE);
        cursor += TIME_SIZE;

        memcpy(signature.data(), cursor, SIG_SIZE);
        cursor += SIG_SIZE;

        return cursor;
    }
    std::byte* marshal(std::byte* cursor) {
        memcpy(cursor, to.data(), KEY_SIZE);
        cursor += KEY_SIZE;

        memcpy(cursor, from.data(), KEY_SIZE);
        cursor += KEY_SIZE;

        memcpy(cursor, file_hash.data(), HASH_SIZE);
        cursor += HASH_SIZE;

        memcpy(cursor, &expiration, TIME_SIZE);
        cursor += TIME_SIZE;

        memcpy(cursor, signature.data(), SIG_SIZE);
        cursor += SIG_SIZE;

        return cursor;
    }
};
static constexpr size_t VOUCHER_SZ = sizeof(Voucher);

