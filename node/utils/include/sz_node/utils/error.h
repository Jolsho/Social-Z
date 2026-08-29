/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_node/msgT.h"
#include "sz_node/actor.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int ErrorCode; 
#define E_SUCCESS          ((ErrorCode)0)
#define E_INTERNAL         ((ErrorCode)1)
#define E_OVERSIZED        ((ErrorCode)2)
#define E_MALFORMED        ((ErrorCode)3)
#define E_UNAUTHORIZED     ((ErrorCode)4)
#define E_NOTLOCAL         ((ErrorCode)5)
#define E_PERM_NOT_EXIST   ((ErrorCode)6)
#define E_FILE_NOT_EXIST   ((ErrorCode)7)
#define E_VOUCHER_EXPIRED  ((ErrorCode)8)
#define E_UNDERSIZED       ((ErrorCode)9)
#define E_BAD_ANON         ((ErrorCode)10)
#define E_BANNED           ((ErrorCode)11)

typedef struct Error {
    int         r;
    ConnID      id;
    int         code;
    Key         key;
    const char*       msg;
} Error;

inline bool is_err(Error* e) { return e->r != 0 || e->code != E_SUCCESS; }

static Error ESUCCESS = {
    .r = 0,
    .id = 0,
    .code = E_SUCCESS,
};

typedef Vec*GetBuff(size_t);

inline size_t error_size(Error* e) {
    return sizeof(int) + sizeof(ConnID) + sizeof(int) + KEY_SIZE + strlen(e->msg);
}

int marshal_error(
    Error* e, 
    Msg* msg, 
    Actors too
);
void unmarshal_error(Error* e, Msg* m);

#ifdef __cplusplus
}
#endif
