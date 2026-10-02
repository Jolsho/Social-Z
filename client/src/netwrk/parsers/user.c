/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "netwrk/context.h"
#include "netwrk/marshalers.h"
#include "netwrk/parsers.h"

int parse_user_data(
    struct Client* cli,
    ContextID id,
    uint8_t* b,
    uint64_t len
) {
    if (!valid_id(id))
        return CLIENT_INVALID_ID;
    if (!cli || cli->login.id != id)
        return CLIENT_ERR;

    int r = CLIENT_ERR;

    if (!b)
        goto done;

    if (!cli->login.fetching) {
        if (len != LOGIN_LOOKUP_SIZE || login_read_lookup(&cli->login.lookup, b, (size_t)len) != 0)
            goto done;
        if (cli->login.lookup_ready)
            goto done;

        cli->login.lookup_ready = true;
        if (cli->net.states[id].send_owned)
            return CLIENT_OK;
        cli->net.states[id].state = CON_IDLE;
        return send_user_data_request(cli, id);
    }
    if (len < HASH_SIZE || len > HASH_SIZE + sizeof(uint64_t) + LOGIN_BLOB_SIZE ||
        memcmp(b, cli->login.lookup.hash.b, HASH_SIZE) != 0)
        goto done;
    if (!cli->net.states[id].blob_active) {
        uint64_t size;
        if (len < HASH_SIZE + sizeof(size))
            goto done;
        memcpy(&size, b + HASH_SIZE, sizeof(size));
        if (size != cli->login.lookup.size)
            goto done;
    }

    r = parse_blob(cli, id, b, len);
    if (r == CLIENT_OK)
        return r;
    if (r != CLIENT_PARSE_DONE)
        goto done;

    StoreItem* item = store_get_item(&cli->blob_store, &cli->login.lookup.hash);
    r = CLIENT_ERR;

    if (!item || item->size != LOGIN_BLOB_SIZE || item->received != item->size)
        goto done;
    if (decrypt_account_header(&cli->account, &cli->keys, &cli->login.lookup.account,
        cli->login.username, cli->login.password.b, cli->login.password.size, item->b, item->size) == 0) {
        cli->logged_in = true;
        r = CLIENT_PARSE_DONE;
    }

done:
    client_free_context(cli, id);

    return r;
}
