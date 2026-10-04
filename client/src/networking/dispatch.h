/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "networking/networker.h"

#define HANDLER_ID_USER_DATA 0
#define HANDLER_ID_BLOB 1
#define HANDLER_ID_PACKAGE 2
#define HANDLER_ID_FEED_PAGE 3
#define HANDLER_ID_CAP 4

extern const HandlerEntry handlers[HANDLER_ID_CAP];
