/*
 * Copyright (c) 2026 Jolsho
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "networking/networker.h"

#define PARSER_ID_USER_DATA 0
#define PARSER_ID_BLOB 1
#define PARSER_ID_PACKAGE 2
#define PARSER_ID_CAP 3

extern const ParserEntry parsers[PARSER_ID_CAP];
