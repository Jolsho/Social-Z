#pragma once
#include <cstdint>

enum Actors : uint8_t {
    PEERNET,
    FILESYS,
    RPC_SERVER,
    SOCIALIZER,
    BLOCKCHAIN,
    LOGGER,

    COUNT,
    NONE,
};

enum Code : uint16_t {
    CTRL        = 0,
    SUCCESS     = 1,
    LOG         = 2,

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
    NEW_SESSION = 2002,


    SOCIAL      = 2500,
    LOGIN       = 2501,
    GET_CHATS   = 2502,
    PUT_CHAT    = 2502,


    BLOCK       = 3000,


    ERRORS              = 6400,
    E_OVERSIZED         = 6401,
    E_INTERNAL          = 6402,
    E_MALFORMED         = 6403,
    E_UNAUTHORIZED      = 6404,
    E_NOTLOCAL          = 6405,
    E_PERM_NOT_EXIST    = 6406,
    E_VOUCHER_EXPIRED   = 6407,
    E_FILE_NOT_EXIST    = 6408,
};

Actors code_too_too(Code c);
