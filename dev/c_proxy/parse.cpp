/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "parse.h"
#include "server.h"
#include <charconv>

//////////////////////////////////////////////////////////////

int parse::on_method(llhttp_t* parser, const char* method, size_t length) {
    auto [server, conn] = http::cast_data(parser);
    if (MX_METH_LEN < (conn.r.method.size() + length)) {
        conn.err.status = Status::METHOD_NOT_ALLOWED;
        conn.err.reason = "Method exceeded acceptable length.";
        return -1;
    }
    std::string& s = conn.r.method;
    size_t old_size = s.size();
    s.resize(old_size + length);
    memcpy(s.data() + old_size, method, length);
    return 0;
}

int parse::on_method_complete(llhttp_t* parser) {
    auto [server, conn] = http::cast_data(parser);
    return 0;
}

//////////////////////////////////////////////////////////////

int parse::on_url(llhttp_t* parser, const char* at, size_t length) {
    auto [server, conn] = http::cast_data(parser);
    if (MX_URL_LEN < (conn.r.base.size() + length)) {
        conn.err.status = Status::URI_TOO_LONG;
        conn.err.reason = "URL too long.";
        return -1;
    }
    std::string& s = conn.r.base;
    size_t old_size = s.size();
    s.resize(old_size + length);
    memcpy(s.data() + old_size, at, length);
    return 0;
}

int parse::on_url_complete(llhttp_t* parser) {
    auto [server, conn] = http::cast_data(parser);
    Request& r = conn.r;
    std::string_view url{r.base.data(), r.base.size()};

    size_t pos = url.find("://");
    if (pos == std::string_view::npos) {
        conn.err.status = Status::BAD_REQUEST;
        conn.err.reason = "URL doesn't include scheme.";
        return -1;
    }

    r.scheme = url.substr(0, pos);
    url.remove_prefix(pos);

    // ---- IPv6 ----
    if (!url.empty() && url.front() == '[') {
        size_t end = url.find(']');
        if (end == std::string_view::npos) {
            conn.err.status = Status::BAD_REQUEST;
            conn.err.reason = "URL doesnt cap ipv6 with ']'.";
            return -1;
        }

        r.host = url.substr(1, end - 1);
        url.remove_prefix(end + 1);
    }
    // ---- IPv4 / domain ----
    else {
        size_t end = url.find_first_of(":/?#");
        r.host = url.substr(0, end);
        url.remove_prefix(end);
    }


    // ---- Port ----
    if (!url.empty() && url.front() == ':') {
        url.remove_prefix(1);

        size_t end = url.find_first_of("/?#");
        std::string_view port_str = url.substr(0, end);

        int value = 0;
        auto [ptr, ec] = std::from_chars(
            port_str.data(), port_str.data() + port_str.size(), value
        );

        if (ec != std::errc() || value < 0 || value > 65535) {
            conn.err.status = Status::BAD_REQUEST;
            conn.err.reason = "Bad choice of port.";
            return -1;
        }

        r.port = static_cast<uint16_t>(value);
        url.remove_prefix(port_str.size());
    }


    // ---- Path ----
    if (url.empty() || url.front() != '/') {
        r.path = { "/" };
    } else {
        size_t end = url.find_first_of("?#");
        r.path = url.substr(0, end);
    }


    // ---- Query ----
    if (!url.empty() && url.front() == '?') {
        url.remove_prefix(1);

        size_t end = url.find('#');
        if (end == std::string_view::npos) end = url.size();

        std::string_view query = url.substr(0, end);
        url.remove_prefix(query.size());


        size_t i = 0;
        const size_t n = query.size();

        size_t total_q_len = 0;

        while (i < n) {
            size_t key_start = i;
            size_t key_end = n;
            size_t val_start = n;
            size_t val_end = n;

            // Walk until '&' or end
            while (i < n && query[i] != '&') {
                if (query[i] == '=' && val_start == n) {
                    key_end = i;
                    val_start = i + 1;
                }
                i++;
            }

            if (key_end == n)
                key_end = i; // no '=' found

            val_end = i;

            if ((total_q_len += ((val_end - key_start) + 1)) > QUERY_LEN) {
                conn.err.status = Status::URI_TOO_LONG;
                conn.err.reason = "Max Query Len Exceeded.";
                return -1;
            }

            std::string_view key = query.substr(key_start, key_end - key_start);
            std::string_view val = (val_start <= val_end && val_start != n)
                ? query.substr(val_start, val_end - val_start)
                : std::string_view{};

            r.query.emplace_back(key, val);

            i++; // skip '&'
        }
    }


    // ---- Fragment ----
    if (!url.empty() && url.front() == '#') {
        url.remove_prefix(1); 
        r.fragment = url;
    }


    return 0;
}

//////////////////////////////////////////////////////////////////////////

int parse::on_header_field(llhttp_t* parser, const char* name, size_t length) {
    auto [server, conn] = http::cast_data(parser);

    size_t new_f_cursor = conn.r.f_cursor + length;
    if (new_f_cursor > (conn.r.in_h.cursor - H_V_LIMIT)) {
        conn.err.status = Status::BAD_REQUEST;
        conn.err.reason = "Header is too long.";
        return -1;
    }
    conn.r.f_cursor = new_f_cursor;

    size_t i{ 0 };
    while (i < length) {
        conn.r.in_h.buff[conn.r.in_h.cursor++] = 
            std::tolower((unsigned char)*(name + i++));
    }
    return 0;
}

int parse::on_header_value(llhttp_t* parser, const char* value, size_t length) {
    auto [server, conn] = http::cast_data(parser);
    if (
        conn.r.v_cursor >= conn.r.in_h.cursor ||
        !conn.r.in_h.write_at(conn.r.v_cursor, value, length)
    ) {
        conn.err.status = Status::BAD_REQUEST;
        conn.err.reason = "Header is too long.";
        return -1;
    }
    return 0;
}

int parse::on_header_complete(llhttp_t* parser) {
    auto [server, conn] = http::cast_data(parser);

    if (conn.r.h_primed) {

        size_t start_f = conn.r.in_h.cursor - (H_K_LIMIT + H_V_LIMIT);
        std::string_view field_str{&conn.r.in_h.buff[start_f], conn.r.f_cursor};

        size_t start_v = start_f + H_K_LIMIT;
        std::string_view value_str{&conn.r.in_h.buff[start_f], conn.r.v_cursor};

        conn.r.headers.push_back({field_str, value_str});

        conn.r.in_h.cursor += (H_K_LIMIT + H_V_LIMIT);
        conn.r.f_cursor = conn.r.in_h.cursor;
        conn.r.v_cursor = conn.r.f_cursor + H_K_LIMIT;
    }

    conn.r.h_primed = !conn.r.h_primed;
    return 0;
}

int parse::on_headers_complete(llhttp_t* parser) {
    auto [server, conn] = http::cast_data(parser);
    conn.inbuf.cursor = 0;

    if (conn.r.get_header("upgrade")) {
        return  HPE_PAUSED_UPGRADE;
    }

    auto len = conn.r.get_header("content-length");
    if (len) {
        conn.r.content_len = atoi(len->data());
        if (conn.r.content_len > MX_BODY_LEN) {
            conn.err.status = Status::BAD_REQUEST;
            conn.err.reason = "Content-Length exceeds max.";
            return -1;
        }
    }

    // Search through headers
    return 0;
}

////////////////////////////////////////////////////////////

int parse::on_body(llhttp_t* parser, const char* body, size_t length) {
    auto [server, conn] = http::cast_data(parser);
    conn.inbuf.write_unbounded(body, length);
    return 0;
}

////////////////////////////////////////////////////////////

int parse::on_message_begin(llhttp_t* parser) {
    auto [server, conn] = http::cast_data(parser);
    conn.inbuf.cursor = QUERY_LEN;

    conn.r.clear();

    conn.err = Status::OK_E;

    return 0;
}

int parse::on_message_complete(llhttp_t* parser) {
    auto [server, conn] = http::cast_data(parser);

    // ---- fill out a msg depending on url ----
    if (server.build_n_send_msg(conn) != 0) return -1;

    llhttp_reset(parser);
    return 0;
}
