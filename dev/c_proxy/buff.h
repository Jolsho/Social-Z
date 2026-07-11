#pragma once
#include <cstring>

template <std::size_t T>
struct Buffer {
    char buff[T];
    size_t cursor;

    inline size_t cap() { return T; }

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
            return;
        }
        throw -1;
    }

    void new_line_thrw() {
        static const char* NEW_LINE = "\r\n";
        static const size_t NEW_LINE_SZ = strlen(NEW_LINE);
        if ((cursor + NEW_LINE_SZ) < T) {
            memcpy(buff + cursor, NEW_LINE, NEW_LINE_SZ);
            cursor += NEW_LINE_SZ;
            return;
        }
        throw -1;

    }
};
