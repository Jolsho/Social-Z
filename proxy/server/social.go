/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package server

import (
	"log"
	"net/http"
)

func api_security(next http.Handler, _ *Server) http.Handler {
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        log.Printf("%s %s", r.Method, r.URL.Path)

        // Call the next handler
        next.ServeHTTP(w, r)
    })
}

func handle_social_requests(_ *Server) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {

/*

The flow here is something like
- Send Request to backend.
- It will reply perhaps with multiple chunks.
- perhaps we make these packet like.
	such that we can keep track of them and like 
	know when to close a response and shit.


So first section is parse the incoming request
then marshal it into a request the db will understand
then send the message and wait response.
when it comes back parse the header of the message 
	to figure out if its multiple chunks or whatever
then just forward the rest to the client

there is the question about DB sending back json...
i dont know whether to do that...
or do I parse the messages with a C funtion?

*/

	})
}
