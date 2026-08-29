/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include "sz_node/actor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SZT SZT;
SZT* new_sz();
int run(SZT* sz);
void stop(SZT* sz);
Actor* new_actor(SZT* sz, ActorConfig* conf);


// THESE PASS MSGS TO SZT ACTORS
// POP FROM SZT MSG STORE AND PUSH TO DESTINATION
HashT new_file(SZT* sz, SSID ssid, size_t total_len);
bool file_chunk(SZT* sz, TrxID trx_id, const uint8_t* data, size_t len);
TrxID get_file(SZT* sz, SSID ssid, HashT* name);

HashT put_card(SZT* sz, SSID ssid, const uint8_t* data, size_t len, int cardType);
TrxID remove_card(SZT* sz, SSID ssid, const char* id);


// Broadcast Voucher for Card.
// list of pubkeys and a single card hash.
// I think you just add something like a broadcast id to the connection.
// so when the message finishes sending then you can access man.broadcast
//      to send to the next user or whatever.

// Request card[] from remote user
// AKA Redeem voucher || Temp Request


#ifdef __cplusplus
}
#endif



