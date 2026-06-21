#pragma once
#include "api/msgT.h"
#include "api/actor.h"
#include "utils/key.h"
#include <functional>
#include <string>

static constexpr int E_SUCCESS          = 0;
static constexpr int E_INTERNAL         = -1;
static constexpr int E_OVERSIZED        = -2;
static constexpr int E_MALFORMED        = -3;
static constexpr int E_UNAUTHORIZED     = -4;
static constexpr int E_NOTLOCAL         = -5;
static constexpr int E_PERM_NOT_EXIST   = -6;
static constexpr int E_FILE_NOT_EXIST   = -7;
static constexpr int E_VOUCHER_EXPIRED  = -8;
static constexpr int E_UNDERSIZED       = -9;
static constexpr int E_BAD_ANON         = -10;
static constexpr int E_BANNED           = -11;

struct Error {
    int         r       = 0;
    ConnID      id      = 0;
    int         code    = E_SUCCESS;
    Key         key     = ZERO_KEY;
    std::string msg;

    inline bool is_err() const {
        return r != 0 || code != E_SUCCESS;
    }
};
const Error ESUCCESS {0, 0, E_SUCCESS, {}};


int marshal_error(
    Error& e, 
    Msg* msg, 
    Actors too,
    std::function<Vec*(size_t)> get_buffer
);
void unmarshal_error(Error& e, Msg* m);
