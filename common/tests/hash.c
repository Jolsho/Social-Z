/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_common/hash.h"
#include <assert.h>

/* First 32 output bytes from BLAKE3's published test vectors.
 * https://github.com/BLAKE3-team/BLAKE3/blob/1.8.5/test_vectors/test_vectors.json
 */
static const struct {
    size_t length;
    const char* hash;
} vectors[] = {
    {0, "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262"},
    {1, "2d3adedff11b61f14c886e35afa036736dcd87a74d27b5c1510225d0f592e213"},
    {1024, "42214739f095a406f3fc83deb889744ac00df831c10daa55189b5d121c855af7"},
    {1025, "d00278ae47eb27b34faecf67b4fe263f82d5412916c1ffd97c8cb7fb814b8444"},
    {2048, "e776b6028c7cd22a4d0ba182a8bf62205d2ef576467e838ed6f2529b85fba24a"},
};

static void check_hash(HashT hash, const char* expected)
{
    const char* digits = "0123456789abcdef";
    for (size_t i = 0; i < HASH_SIZE; i++) {
        assert(digits[hash.b[i] >> 4] == expected[2 * i]);
        assert(digits[hash.b[i] & 15] == expected[2 * i + 1]);
    }
}

int main(void)
{
    uint8_t input[2048];
    for (size_t i = 0; i < sizeof(input); i++) input[i] = i % 251;

    for (size_t v = 0; v < sizeof(vectors) / sizeof(vectors[0]); v++) {
        Hasher whole = new_hasher();
        hash_update(&whole, input, vectors[v].length);
        check_hash(hash_finalize(&whole), vectors[v].hash);
        check_hash(hash_finalize(&whole), vectors[v].hash);

        Hasher chunks = new_hasher();
        for (size_t offset = 0; offset < vectors[v].length;) {
            size_t length = vectors[v].length - offset;
            if (length > 63) length = 63;
            hash_update(&chunks, input + offset, length);
            offset += length;
        }
        check_hash(hash_finalize(&chunks), vectors[v].hash);
    }

    Hasher original = new_hasher();
    hash_update(&original, input, 1);
    Hasher copy = original;
    hash_update(&original, input + 1, sizeof(input) - 1);
    check_hash(hash_finalize(&copy), vectors[1].hash);
    check_hash(hash_finalize(&original), vectors[4].hash);
    return 0;
}
