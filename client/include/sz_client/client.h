
/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


// OUTPUT CODES
#define CLIENT_PARSE_DONE   1
#define CLIENT_OK           0
#define CLIENT_ERR          -1
#define CLIENT_CONN_BUSY    -2
#define CLIENT_SMALL_BUFFER -3
#define CLIENT_INVALID_ID   -4


/// THE ROOT OF ALL EVIL
struct Client;
struct Client* init_client();


/// An ID for an abstraction over a connection.
typedef int16_t ContextID;


/// Buffer is used as a reference to WASM buffers.
/// They represent ownership of a block of memory.
struct Buffer;
void client_return_buffer(struct Client* cli, struct Buffer* buff);


/// Implementors are given ownership of the buffer and must manage.
/// Expected to be asynchronous and implementors can call client_parse_response() after.
extern void send_request(struct Client* cli, ContextID id, struct Buffer* buff);


/// Can pass any response from a node into this and it will return CLIENT OUTPUT CODES.
/// This borrows a view of memory so client does not assume ownership.
int client_parse_response(struct Client*, ContextID id, uint8_t* b, uint64_t l);


/* INPUT */
struct InputState;
void input_set_mouse_pos_n_scroll(struct InputState*, int x, int y, float wheel);
void input_press_mouse_btn(struct InputState*, uint8_t key);
void input_release_mouse_btn(struct InputState*, uint8_t key);
void input_press_key(struct InputState*, uint8_t key);
void input_release_key(struct InputState*, uint8_t key);
void input_reset(struct InputState*);


/* UI STATE & RENDERER */
void client_set_input_state(struct Client*, struct InputState*);
bool client_update_wrld(struct Client*);
void client_render_frame(struct Client*);

/*
 *      JS IMPLEMENTATION
 *      -------------------
 *
 *      client = init_client();
 *
 *      function continue() {
 *
 *          client_set_input(client, x, y, keys, keys_len);
 *
 *          client_update_state(client)
 *
 *          client_render_frame(client);
 *      
 *          requestAnimationFrame(continue) 
 *      }
 *      continue();
*/


#ifdef __cplusplus
}
#endif
