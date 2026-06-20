#pragma once
#include "llhttp.h"

namespace parse {

// Append the URL chunk to your request’s URL buffer.
int on_url(llhttp_t* parser, const char* at, size_t length);
// Sanitize the url and make sure path exists
int on_url_complete(llhttp_t* parser);

// Store which HTTP method (GET, POST, etc.) is being used.
// (You can also get this via parser->method instead of a callback.)
int on_method(llhttp_t* parser, const char* method, size_t length);
// Sanitize the method
int on_method_complete(llhttp_t* parser);


// Append to a temporary “current header name” buffer 
// (handles split fields).
int on_header_field(llhttp_t* parser, const char* name, size_t length);
// Append to a temporary “current header value” buffer.
int on_header_value(llhttp_t* parser, const char* value, size_t length);
// Store the completed header key/value pair in your request structure and reset temp buffers.
int on_header_complete(llhttp_t* parser);
// Decide how to handle the body (e.g., check Content-Length, Transfer-Encoding, or WebSocket upgrade).
int on_headers_complete(llhttp_t* parser);


// Append the received body chunk to your request body buffer 
// (this works for both normal and chunked bodies).
int on_body(llhttp_t* parser, const char* body, size_t length);


// Initialize/reset per-request state
// clear headers, buffers, flags
int on_message_begin(llhttp_t* parser);
// Finalize the request and hand it off to your 
// application logic (routing, response, or WebSocket upgrade).
int on_message_complete(llhttp_t* parser);

};
