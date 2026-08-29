/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLIENT_PARSE_DONE   1
#define CLIENT_OK           0
#define CLIENT_ERR          -1
#define CLIENT_CONN_BUSY    -2
#define CLIENT_SMALL_BUFFER -3
#define CLIENT_INVALID_ID   -4

struct Client;

typedef int16_t ContextID;

#define ID_CAP 32767

struct Client* init_client_state();

ContextID client_new_context(struct Client* cli);
void client_free_context(struct Client* cli, ContextID id);

int client_parse_response(struct Client* cli, ContextID id, uint8_t* b, uint64_t l);

#ifdef __cplusplus
}
#endif
