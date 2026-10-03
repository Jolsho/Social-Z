/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "sz_client/client.h"
#include "sz_common/requests/requests.h"
#include "utils/buffers.h"
#include "codec/blob.h"

typedef struct LoginOperation {
    BlobTransfer blob;
    bool metadata_ready;
    char username[ACCOUNT_USERNAME_MAX + 1];
    Buffer password;
    AccountResponseMetadata metadata;
} LoginOperation;

// Response dispatch advances the username request through metadata and header chunks.
int login_handle_response(struct Client* cli, ContextID id, uint8_t* bytes, uint64_t size);
void login_cleanup(struct Client* cli, ContextID id);
