/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "client.h"
#include <stdlib.h>

int parse_blob(Client* cli, ContextID id, uint8_t* b, uint64_t len) {
    HashT h;
    StoreItem* item;

    memcpy(&h, b, HASH_SIZE);
    b += HASH_SIZE;
    len -= HASH_SIZE;

    if ((item = store_get_item(&cli->blob_store, &h)) == NULL) {

        uint64_t blob_size = 0;
        memcpy(&blob_size, b, sizeof(uint64_t));
        b += sizeof(uint64_t);

        item = store_assign_item(&cli->blob_store, &h, b, blob_size, &cli->pool);
        if (!item) return CLIENT_ERR;
    }



    return CLIENT_PARSE_DONE;
}
