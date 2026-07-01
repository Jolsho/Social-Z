package server

import (
	"log"
	"net/http"
)

func admin_security(next http.Handler, _ *Server) http.Handler {
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        log.Printf("%s %s", r.Method, r.URL.Path)

        // Call the next handler
        next.ServeHTTP(w, r)
    })
}

func handle_admin_requests(_ *Server) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {

	})
}
