#pragma once
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t DB_PATH; 
#define DB_USER_INSERT      0
#define DB_USER_DELETE      1
#define DB_CARD_RECENT      2
#define DB_PERM_INSERT      3
#define DB_PERM_DELETE      4
#define DB_VOUCHER_INSERT   5
#define DB_NEW_BLOB         6

typedef uint8_t FS_PATH; 
#define FS_VOUCHER  0
#define FS_REDEEM   1
#define FS_REWARD   2
#define FS_GIVE     3
#define FS_SETTLE   4
#define FS_ASK      5
#define FS_REVOKE   6

typedef uint8_t LOG_PATH; 
#define LOG_LOG  0

typedef uint8_t P2P_PATH; 
#define P2P_CLOSE_CONN  0
#define P2P_NEW_CONN    1
#define P2P_BROADCAST   2
#define P2P_PING        3
#define P2P_PONG        4

typedef uint8_t RPC_PATH;
#define NOTI_CARD       1
#define NOTI_GIVE       2
#define NOTI_ACCEPTED   3 
#define NOTI_DECLINED   4
#define NOTI_PERM_REQ   5

#ifdef __cplusplus
}
#endif
