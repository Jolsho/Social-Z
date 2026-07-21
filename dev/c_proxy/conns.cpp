/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "conns.h"
#include <cstring>

int conn_t::drive_tls_handshake() {
    int r = SSL_accept(ssl);

    if (r != 1) {
        static constexpr int MAX_RETRY = 3;
        if (++hand_failures > MAX_RETRY) {
            err.status = Status::REQUEST_TIMEOUT;
            err.reason = "Failed to handshake.";
            return -2;
        }
        return -1;
    }

    int err = SSL_get_error(ssl, r);
    if (err == SSL_ERROR_WANT_READ) return 0;
    if (err == SSL_ERROR_WANT_WRITE) return 0;
    return err;
}

int conn_t::read_() {

    while (1) {
        int n = SSL_read(ssl, inbuf.buff, inbuf.cap() - 1 - inbuf.cursor);

        if (n > 0) {
            inbuf.cursor += n;
            if (!is_ws) {
                auto e = llhttp_execute(&parser, inbuf.buff + (inbuf.cursor - n), n);
                switch (e) {
                    case HPE_OK:
                    case HPE_PAUSED:  
                        break;

                    case HPE_PAUSED_UPGRADE:
                    case HPE_PAUSED_H2_UPGRADE:
                        is_ws = true;
                        break;

                    default: {
                        if (err.is_ok()) {
                            err.status = Status::BAD_REQUEST;
                            err.reason = "Unknown HTTP ERR.";
                        }
                        return -1;
                    }
                }
            } else {
                if (!parse_ws(n)) return -1;
            }
        }

        int err = SSL_get_error(ssl, n);
        if (err == SSL_ERROR_WANT_READ) break;
        if (err == SSL_ERROR_WANT_WRITE) break;
        if (err == SSL_ERROR_ZERO_RETURN) {
            return err;
        }
        break;
    }
    return 0;
}

int conn_t::write_() {
    int n;
    if (!written_h) {
        n = SSL_write(ssl, out_h.buff, out_h.cursor);
        if (n > 0) {
            memmove(out_h.buff, out_h.buff + n, out_h.cursor - n);
            out_h.cursor -= n;
            if (out_h.cursor == 0) written_h = true;
            return 0;
        }
    } else {
        n = SSL_write(ssl, outbuf.buff, outbuf.cursor);
        if (n > 0) {
            memmove(outbuf.buff, outbuf.buff + n, outbuf.cursor - n);
            outbuf.cursor -= n;
            if (outbuf.cursor == 0) written_h = false;
            return 0;
        }
    }


    int err = SSL_get_error(ssl, n);
    if (err == SSL_ERROR_WANT_WRITE) return 0;
    if (err == SSL_ERROR_WANT_READ) return 0;

    return err;
}

std::string make_http_date() {
    time_t now = time(nullptr);

    tm tm{};
    gmtime_r(&now, &tm);

    char buf[64];
    strftime(buf, sizeof(buf),
                  "%a, %d %b %Y %H:%M:%S GMT",
                  &tm);

    return std::string(buf);
}

void conn_t::queue_err() {
    // TODO
}

int conn_t::queue_response(Msg* msg) {

    if (res_q.front() != msg->mid) {
        outbound_msgs.push_back(msg);
        return 0;
    }
    res_q.pop();

    char* cursor = (char*)msg->data;
    std::string_view status = {cursor, strlen(cursor)};
    cursor += status.size();
    std::string_view reason = {cursor, strlen(cursor)};
    cursor += reason.size();

    uint8_t header_count;
    memcpy(&header_count, cursor, sizeof(uint8_t));
    cursor += sizeof(uint8_t);

    uint16_t header_len;
    memcpy(&header_len, cursor, sizeof(uint16_t));
    cursor += sizeof(uint16_t);

    char* h_cursor = cursor;
    cursor += header_len;

    uint16_t body_len;
    memcpy(&body_len, cursor, sizeof(uint16_t));
    cursor += sizeof(uint16_t);

    try {
        out_h.append_str_thrw("HTTP/1.1 ");

        out_h.append_str_thrw(status.data(), status.length());

        out_h.append_str_thrw(" ", 1);
        out_h.append_str_thrw(reason.data(), reason.size());
        out_h.new_line_thrw();


        // Required headers
        std::string date = make_http_date();
        out_h.append_str_thrw("Date: ", 6);
        out_h.append_str_thrw(date.data(), date.size());
        out_h.new_line_thrw();

        out_h.append_str_thrw("Content-Length: ");
        std::string len = std::to_string(body_len);
        out_h.append_str_thrw(len.data(), len.length());
        out_h.new_line_thrw();

        // Custom headers
        for (int i{0}; i < header_count; i++) {
            std::string_view field = {h_cursor, strlen(h_cursor)};
            h_cursor += field.size();
            out_h.append_str_thrw(field.data(), field.size());
            out_h.append_str_thrw(": ", 2);

            std::string_view value = {h_cursor, strlen(h_cursor)};
            h_cursor += value.size();
            out_h.append_str_thrw(value.data(), value.size());
            out_h.new_line_thrw();
        }

        // End of headers
        out_h.new_line_thrw();

        // Body
        outbuf.append_str_thrw(cursor, body_len);

        return 0;
    } catch (int c) {
        return c;
    }
}
