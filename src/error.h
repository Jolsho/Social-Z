#pragma once
#include "codes.h"
#include "msg.h"
#include <string>

struct Error {
    int         r       = -1;
    ConnID      id      = 0;
    Code        code    = Code::E_INTERNAL;
    std::string msg;

    inline bool is_err() const {
        return r != 0 || code != Code::SUCCESS;
    }
};
const Error ESUCCESS {0, 0, Code::SUCCESS, {}};


