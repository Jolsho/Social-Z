#pragma once
#include "msg.h"
#include <cstring>
#include <string>

template <std::size_t T>
struct Buffer {
    char buff[T];
    size_t cursor;

    bool write(const char* src, size_t len) {
        if ((cursor + len) < T) {
            memcpy(buff + cursor, src, len);
            cursor += len;
            return true;
        }
        return false;
    }

    bool write_at(size_t& at, const char* src, size_t len) {
        if ((at + len) < T) {
            memcpy(buff + at, src, len);
            at += len;
            return true;
        }
        return false;
    }

    void write_unbounded(const char* src, size_t len) {
        memcpy(buff + cursor, src, len);
        cursor += len;
    }

    void append_str_thrw(const char* src, int len = -1) {
        if (len == -1) {
            len = strlen(src);
        }
        if ((cursor + len) < T) {
            memcpy(buff + cursor, src, len);
            cursor += len;
        }
        throw -1;
    }

    void new_line_thrw() {
        static const char* NEW_LINE = "\r\n";
        static const size_t NEW_LINE_SZ = strlen(NEW_LINE);
        if ((cursor + NEW_LINE_SZ) < T) {
            memcpy(buff + cursor, NEW_LINE, NEW_LINE_SZ);
            cursor += NEW_LINE_SZ;
        }
        throw -1;

    }


    inline size_t size() { return T; }
};


using StrPairs = std::vector<std::pair<std::string_view, std::string_view>>;
namespace Status {

static constexpr const char* OK                     {"200"};
static constexpr const char* CREATED                {"201"};
static constexpr const char* NO_CONTENT             {"204"};

static constexpr const char* BAD_REQUEST            {"400"};
static constexpr const char* UNAUTHORIZED           {"401"};
static constexpr const char* FORBIDDEN              {"403"};
static constexpr const char* NOT_FOUND              {"404"};
static constexpr const char* METHOD_NOT_ALLOWED     {"405"};
static constexpr const char* REQUEST_TIMEOUT        {"408"};
static constexpr const char* PAYLOAD_TOO_LARGE      {"413"};
static constexpr const char* URI_TOO_LONG           {"414"};

static constexpr const char* INTERNAL_ERROR         {"500"};
static constexpr const char* NOT_IMPLEMENTED        {"501"};
static constexpr const char* BAD_GATEWAY            {"502"};
static constexpr const char* SERVICE_UNAVAILABLE    {"503"};

struct Error {
    ConnID              id      = 0;
    std::string_view    status  = OK;
    std::string         reason  = "OK";

    inline bool is_ok() { return status == OK; }
};

static constexpr Error OK_E {0, OK, "OK"};


} // namespace HTTP_RES
