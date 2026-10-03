/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "sz_client/client.h"
#include "sz_common/codec.h"
#include "content/feed.h"

// Start a blob GET and load one encrypted feed page using its data key.
// The caller supplies its storage label; page_number checks the decrypted page's identity.
// destination must remain alive at the same address until completion/cancellation.
// Only a verified, parsed page changes the feed; current navigation stays unchanged.
int feed_page_get(
    struct Client* cli,
    const Key* owner,
    const HashT* label,
    const Key* key,
    uint64_t page_number,
    Feed* destination,
    ContextID* id
);

int feed_page_handle_response(struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size);
void feed_page_cleanup(struct Client* cli, ContextID id);
