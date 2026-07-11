package server

import (
	"log"
	"net/http"
	"sz_proxy/sz"
	"time"
)

type Server struct {
	from_rpc 	chan sz.Msg
	to_rpc 		chan sz.Msg
	waiting 	map[uint16]chan sz.Msg
	buffers		*sz.BufferStore
};


func file_rate_limit(next http.Handler, _ *Server) http.Handler {
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        log.Printf("%s %s", r.Method, r.URL.Path)

        // Call the next handler
        next.ServeHTTP(w, r)
    })
}

func Start_server(from_rpc chan sz.Msg, to_rpc chan sz.Msg, buffers *sz.BufferStore) {

	s := Server{ 
		from_rpc: from_rpc, 
		to_rpc: to_rpc, 
		waiting: make(map[uint16]chan sz.Msg, 256),
		buffers: buffers,
	}


	// Api routes
	api := http.NewServeMux()
	api.Handle("/social", 	handle_social_requests(&s))

	// Admin routes
	admin := http.NewServeMux()
	admin.Handle("/", 		handle_admin_requests(&s))

	// Root router
	root := http.NewServeMux()
	root.Handle("/api/", 	http.StripPrefix("/api", 	api_security(api, &s)))
	root.Handle("/admin/", 	http.StripPrefix("/admin", 	admin_security(admin, &s)))
	root.Handle("/", 		file_rate_limit(http.FileServer(http.Dir("../web_ui/dist")), &s))

	server := http.Server{
		Handler: 			root,
		Addr: 				"127.0.0.1:8080",
		ReadHeaderTimeout: 	time.Duration(time.Duration(10).Milliseconds()),
		ReadTimeout: 		time.Duration(time.Duration(10).Milliseconds()),
		WriteTimeout: 		time.Duration(time.Duration(10).Milliseconds()),
		IdleTimeout: 		time.Duration(time.Duration(100).Milliseconds()),
	}
	err := server.ListenAndServeTLS("./sz.cert", "./sz.key")
	if err != nil {

	}
}
