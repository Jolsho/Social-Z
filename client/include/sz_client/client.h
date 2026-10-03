
/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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
// Initializes core client state; rendering startup is separate.
struct Client* init_client(void);
void destroy_client(struct Client* cli);


/// An ID for an abstraction over a connection.
typedef int16_t ContextID;


// Login copies the password locally. A fresh client is required to switch accounts.
int client_login(struct Client* cli, const char* username,
    const uint8_t* password, size_t password_size, ContextID* id);
int client_cancel_login(struct Client* cli);
bool client_is_logged_in(const struct Client* cli);
int client_get_public_key(const struct Client* cli, uint8_t public_key[32]);

/// Buffer is used as a reference to WASM buffers.
/// They represent ownership of a block of memory.
struct Buffer;
// Return the original outgoing buffer once the host no longer needs its bytes.
// A response does not release the outgoing buffer.
// Return outstanding buffers before destroying the client.
void client_return_buffer(struct Client* cli, struct Buffer* buff);


// Host hooks return CLIENT_OK when accepted, CLIENT_ERR on immediate failure.
// begin opens one logical request; writes append bytes; end closes its body.
// begin/write must not deliver responses or call client_request_failed.
// end may deliver a response synchronously; otherwise the host supplies it later.
extern int request_begin(struct Client* cli, ContextID id);
extern int request_write(struct Client* cli, ContextID id, struct Buffer* buff);
extern int request_end(struct Client* cli, ContextID id);

// A successful write retains the original buffer until client_return_buffer.
// A failed write retains nothing. Only one buffer per context may be outstanding.
// end may be called while that buffer is retained; it must follow its bytes.
// An accepted end returns CLIENT_OK even if a synchronous response fails parsing.

// Abort stops further delivery for this context. It may return the send buffer,
// or retain it until later, but must not deliver responses or failure callbacks.
extern void request_abort(struct Client* cli, ContextID id);
int client_cancel_request(struct Client* cli, ContextID id);

// Report a later transport failure after stopping further delivery for this context.
// Failure cleans operation state; the host must still return any retained buffer.
int client_request_failed(struct Client* cli, ContextID id);


/// Can pass any response from a node into this and it will return CLIENT OUTPUT CODES.
/// This borrows a view of memory so client does not assume ownership.
// On completion or error, stop delivering events for this context.
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
