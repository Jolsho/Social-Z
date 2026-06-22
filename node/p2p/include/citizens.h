#pragma once
#include "utils/error.h"
#include "crypto.h"
#include <cassert>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <unordered_map>


class Citizen {
private:
    static constexpr uint64_t THRESHOLD = 175;
public:
    uint64_t    reputation_;
    time_t      rep_reset_;
    char        ip_v4[16];
    uint16_t    port_;

    void record_infringement(int error) {
        uint64_t score;
        switch (error) {
            case E_OVERSIZED: score = 50;
            case E_MALFORMED: score = 10;
            case E_UNAUTHORIZED: score = 30;
            default: score = 0;
        }
        if (score == 0) return;

        time_t now = time(nullptr);
        if (now > rep_reset_) {
            // REP RESETS EVERY WEEK
            rep_reset_ = now + (60 * 60 * 24 * 7);
            reputation_ = 0;
        }

        reputation_ += score;
    }

    inline bool is_trustworthy() { return reputation_ < THRESHOLD; }
};

class CitizenMap {
    std::string path_;
public:
    std::unordered_map<Key, Citizen, KeyHash> map_;

    Error load(const std::string path) {
        path_ = path;

        int fd = open(path_.c_str(), O_RDWR | O_CREAT, 0600);
        if (fd < 0) {
            return {
                .r = fd,
                .msg = "Failed to open Citizen file."
            };
        }

        lseek(fd, 0, SEEK_SET);

        size_t len = 0;
        size_t n = read(fd, &len, sizeof(len));
        if (n < 0) {
            return {
                .r = fd,
                .msg = "Read Citizen File Failed."
            };
        }

        if (len == 0) return ESUCCESS;
        map_.reserve(len);

        Key k;
        Citizen tmp;
        size_t csz = sizeof(Citizen);
        for (int i = 0; i < len; i++) {
            if (read(fd, &k, KEY_SIZE) < KEY_SIZE) break;
            if (read(fd, &tmp, csz) < csz) break;
            map_.insert({k, tmp});
        }

        close(fd);

        return ESUCCESS;
    }

    Error persist() {
        int fd = open(path_.c_str(), O_RDWR | O_CREAT, 0600);
        if (fd < 0) {
            return {
                .r = fd,
                .msg = "cizizens_persist() :: Failed to open Citizen file."
            };
        }

        lseek(fd, 0, SEEK_SET);

        size_t len = map_.size();
        if (write(fd, &len, sizeof(size_t)) < sizeof(size_t)) {
            close(fd);
            return {
                .r = -1,
                .msg = "cizizens_persist() :: Failed to write citizen length."
            };
        }
        if (len == 0) return ESUCCESS;

        int n;
        for (const auto& [k, c]: map_) {
            n = write(fd, &k, KEY_SIZE);
            if (n != KEY_SIZE) {
                close(fd);
                return {
                    .r = -1,
                    .msg = "citizens_persist() :: Failed to write key."
                };
            }

            n = write(fd, &c, sizeof(Citizen));
            if (n != sizeof(Citizen)) {
                close(fd);
                return {
                    .r = -1,
                    .msg = "citizens_persist() :: Failed to write citizen."
                };
            }

            n = 0;
        }
        close(fd);

        return ESUCCESS;
    }
};
