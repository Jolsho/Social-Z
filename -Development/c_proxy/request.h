/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "buff.h"
#include "misc.h"

static constexpr size_t QUERY_LEN = 256;
static constexpr size_t MX_URL_LEN = QUERY_LEN + 64;
static constexpr size_t MX_METH_LEN = 16;
static constexpr size_t H_K_LIMIT = 32;
static constexpr size_t H_V_LIMIT = 128;
static constexpr size_t H_KV_LIMIT = 25;
static constexpr size_t MX_HEADER_LEN = MX_URL_LEN + MX_METH_LEN + ((H_K_LIMIT + H_V_LIMIT) * H_KV_LIMIT);
static constexpr size_t MX_BODY_LEN = 8192;

struct Request {
    StrPairs                headers;
    Buffer<MX_HEADER_LEN>   in_h;
    size_t                  f_cursor = 0;
    size_t                  v_cursor = H_K_LIMIT;
    bool                    h_primed;

    std::string         method;

    std::string         base;
    std::string_view    scheme;
    std::string_view    host;
    uint16_t            port;
    std::string_view    path;
    StrPairs            query;
    std::string_view    fragment;
    size_t              content_len;

    Buffer<MX_BODY_LEN> body;

    Request() : port{0}, content_len{0} {
        base.reserve(MX_URL_LEN);
        method.reserve(MX_METH_LEN);
        headers.reserve(H_KV_LIMIT);
    }

    inline std::string_view* get_header(const char* name) {
        for (auto& [n, v]: headers) if (n == name) return &v;
        return nullptr;
    }

    void clear() {
        headers.clear();
        in_h.cursor = (H_K_LIMIT + H_V_LIMIT);
        f_cursor = 0;
        v_cursor = H_K_LIMIT;
        h_primed = false;

        method.clear();

        base.clear();
        scheme = "";
        host = "";
        port = 0;
        path = "";
        query.clear();
        fragment = "";
        content_len = 0;
    }
};
