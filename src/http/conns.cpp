#include "llhttp.h"
#include "openssl/ssl.h"
#include "http/conns.h"
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>

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
        int n = SSL_read(ssl, inbuf.buff, inbuf.size() - inbuf.cursor);

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

    int n = SSL_write(ssl, outbuf.buff, outbuf.cursor);

    if (n > 0) {
        memmove(outbuf.buff, outbuf.buff + n, outbuf.cursor - n);
        outbuf.cursor -= n;
        return 0;
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

int conn_t::queue_response(
    std::string_view body,
    std::vector<std::pair<std::string_view, std::string_view>>* headers,
    Status::Error status
) {
    try {
        outbuf.append_str_thrw("HTTP/1.1 ");

        outbuf.append_str_thrw(status.status.data(), status.status.length());

        outbuf.append_str_thrw(" ", 1);
        outbuf.append_str_thrw(status.reason.data(), status.reason.size());
        outbuf.new_line_thrw();


        // Required headers
        std::string date = make_http_date();
        outbuf.append_str_thrw("Date: ", 6);
        outbuf.append_str_thrw(date.data(), date.size());
        outbuf.new_line_thrw();

        outbuf.append_str_thrw("Content-Length: ");
        std::string len = std::to_string(body.size());
        outbuf.append_str_thrw(len.data(), len.length());
        outbuf.new_line_thrw();

        // Custom headers
        if (headers) {
            for (const auto& [key, value] : *headers) { 
                outbuf.append_str_thrw(key.data(), key.size());
                outbuf.append_str_thrw(": ", 2);
                outbuf.append_str_thrw(value.data(), value.size());
                outbuf.new_line_thrw();
            }
        }

        // End of headers
        outbuf.new_line_thrw();

        // Body
        outbuf.append_str_thrw(body.data(), body.size());

        return 0;
    } catch (int c) {
        return c;
    }
}
