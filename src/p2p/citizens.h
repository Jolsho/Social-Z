#pragma once
#include "codes.h"
#include <ctime>

class Citizen {
private:
    static constexpr uint64_t THRESHOLD = 175;
public:
    uint64_t    reputation_;
    time_t      rep_reset_;

    void record_infringement(Code error) {
        uint64_t score;
        switch (error) {
            case Code::E_OVERSIZED: score = 50;
            case Code::E_MALFORMED: score = 10;
            case Code::E_UNAUTHORIZED: score = 30;
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

    bool is_trustworthy() {
        return reputation_ < THRESHOLD;
    }
};
