/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t DB_PATH; 
#define DB_USER_INSERT      ((DB_PATH)0)
#define DB_USER_DELETE      ((DB_PATH)1)
#define DB_CARD_RECENT      ((DB_PATH)2)
#define DB_PERM_INSERT      ((DB_PATH)3)
#define DB_PERM_DELETE      ((DB_PATH)4)
#define DB_VOUCHER_INSERT   ((DB_PATH)5)
#define DB_NEW_BLOB         ((DB_PATH)6)
#define DB_NEW_TASK         ((DB_PATH)7)

typedef uint8_t FS_PATH; 
#define FS_VOUCHER  ((FS_PATH)0)
#define FS_REDEEM   ((FS_PATH)1)
#define FS_REWARD   ((FS_PATH)2)
#define FS_GIVE     ((FS_PATH)3)
#define FS_SETTLE   ((FS_PATH)4)
#define FS_ASK      ((FS_PATH)5)
#define FS_REVOKE   ((FS_PATH)6)

typedef uint8_t LOG_PATH; 
#define LOG_LOG  ((LOG_PATH)0)

typedef uint8_t P2P_PATH; 
#define P2P_CLOSE_CONN  ((P2P_PATH)0)
#define P2P_NEW_CONN    ((P2P_PATH)1)
#define P2P_BROADCAST   ((P2P_PATH)2)
#define P2P_PING        ((P2P_PATH)3)
#define P2P_PONG        ((P2P_PATH)4)

typedef uint8_t RPC_PATH;
#define NOTI_CARD       ((RPC_PATH)1)
#define NOTI_GIVE       ((RPC_PATH)2)
#define NOTI_ACCEPTED   ((RPC_PATH)3)
#define NOTI_DECLINED   ((RPC_PATH)4)
#define NOTI_PERM_REQ   ((RPC_PATH)5)

#ifdef __cplusplus
}
#endif
