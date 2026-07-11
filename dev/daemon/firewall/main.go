package firewall

import (
	"github.com/google/nftables"
)

func Run() {
	s := init_session()
	init_ssh(s)
	listen_Socket(s)
}

type Session struct {
	Conn 	*nftables.Conn
	Logger 	*Logger
	Db 		*Database
}

func init_session() *Session {
	s := new(Session)
	s.Conn = new(nftables.Conn)
	s.Logger = init_logger()
	return s
}
