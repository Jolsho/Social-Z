package firewall

import (
	"daemon/docker/utils"
	"encoding/json"
	"io"
	"log"
	"net"
	"os"
)

const (
	NFT_SOCKET = utils.HOST_SOCKETS + "/nft.sock"

	NFT_NEW int = iota
	NFT_BAN
	NFT_UNBAN

)

func listen_Socket(s *Session) {
	// Remove existing socket file if it exists
	if _, err := os.Stat(NFT_SOCKET); err == nil {
		os.Remove(NFT_SOCKET)
	}

	l, err := net.Listen("unix", NFT_SOCKET)
	if err != nil {
		log.Fatalf("listen error: %v", err)
	}
	defer l.Close()
	log.Printf("Listening on unix socket: %s", NFT_SOCKET)

	for {
		conn, err := l.Accept()
		if err != nil {
			log.Printf("accept error: %v", err)
			continue
		}

		if s.Db == nil {
			init_db(s)
			if s.Db == nil { 
				conn.Close()
				// if database isnt up continue
				// errors were already logged
				// maybe next time it will be up
				continue
			}
			init_citizens(s)
		}

		go Handle_Connection(s, conn)
	}
}

type Message struct {
	Code int             `json:"header"`
	Payload   json.RawMessage `json:"body"`
}

func Handle_Connection(s *Session, conn net.Conn) {
	defer conn.Close()

	decoder := json.NewDecoder(conn)

	for {
		var msg Message
		err := decoder.Decode(&msg)
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Printf("decode error: %v", err)
			break
		}

		switch msg.Code {
		case NFT_NEW: s.new_citizen(&msg.Payload)
		case NFT_BAN: s.ban_citizen(&msg.Payload)
		case NFT_UNBAN: s.unban_citizen(&msg.Payload)
		default:
			log.Printf("Unhandled header: %d", msg.Code)
		}
	}
}

