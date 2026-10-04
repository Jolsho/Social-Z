/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#pragma once

#include "client.h"

static int init_test_networker(struct Client* cli) {
    int result = init_networker(&cli->net);
    cli->net.pool = &cli->pool;
    cli->net.client = cli;
    return result;
}
