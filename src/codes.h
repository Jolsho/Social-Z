#pragma once
#include <cstdint>

enum Actors : uint8_t {
    PEERNET,
    FILESYS,
    RPC_SERVER,
    SOCIALIZER,
    BLOCKCHAIN,

    COUNT,
    NONE,
};

enum CODE : uint16_t {
    CTRL        = 0,
    SUCCESS     = 1,

    INTERNAL    = 500,
    SHUTDOWN    = 501,
    CLOSE_CONN  = 502,
    NEW_CONN    = 503,
    WRITE       = 504,
    

    P2P         = 1000,
    SYN         = 1001,
    SYNACK      = 1002,
    ACK         = 1003,


    FS          = 1500,
    VOUCHER     = 1501,
    REDEEM      = 1502,
    REWARD      = 1503,
    GIVE        = 1504,
    ACCEPT      = 1505,
    ASK         = 1506,
    REVOKE      = 1507,


    RPC         = 2000,
    INDEX       = 2001,

    SOCIAL      = 2500,
    CHATS       = 2501,

    BLOCK       = 3000,

    ERRORS              = 3500,
    E_OVERSIZED         = 3501,
    E_INTERNAL          = 3502,
    E_MALFORMED         = 3503,
    E_UNAUTHORIZED      = 3504,
    E_NOTLOCAL          = 3505,
    E_PERM_NOT_EXIST    = 3506,
    E_VOUCHER_EXPIRED   = 3507,
    E_FILE_NOT_EXIST    = 3508,
};

Actors code_too_too(CODE c);
