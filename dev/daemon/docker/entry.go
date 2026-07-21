/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package docker

import (
	"encoding/json"
	"io"
	"log"
	"net"
	"os"

	"daemon/docker/models"
	"daemon/docker/utils"
)

const (
	DAEMON_SOCKET = utils.HOST_SOCKETS + "/socialz_daemon.sock"
)

func Run(logger *utils.Logger) {
	s, err := models.New_Session(logger)
	if err != nil { 
		s.Logger.Log_err("NewClientWithOpts", err)
		return
	}

	// Start processing connections on SOCKET
	go func() {
		_, err := os.Stat(DAEMON_SOCKET)
		if err == nil { os.Remove(DAEMON_SOCKET) }

		l, err := net.Listen("unix", DAEMON_SOCKET)
		if err != nil { log.Fatalf("listen error: %v", err) }

		defer l.Close()

		for {
			conn, err := l.Accept()
			if err != nil {
				log.Fatalf("accept error: %v", err)
				continue
			}

			go func() {
				defer conn.Close()

				decoder := json.NewDecoder(conn)

				msg := new(utils.Message)
				for {
					err := decoder.Decode(msg)
					if err == io.EOF { break }
					if err != nil {
						log.Printf("decode error:  %v", err)
						break
					}

					s.Handler(conn, msg)

					msg.Code = -1
					msg.Payload = nil
				}
			}()
		}
	}()
}


