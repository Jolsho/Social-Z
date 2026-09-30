/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package firewall

import (
	"database/sql"
	"errors"
	"fmt"
	"net"
	"os"
	"time"

	"github.com/google/nftables"
	_ "github.com/lib/pq"
)

// TODO -- change this to sqlite
// and just handle commands coming from the wallet
type Database struct {
	*sql.DB
}

var (
	HOST = os.Getenv("POSTGRES_HOST")
	PORT = os.Getenv("POSTGRES_PORT")
	UNAME = os.Getenv("POSTGRES_USER")
	PSWRD = os.Getenv("POSTGRES_PASSWORD")
	DNAME = os.Getenv("POSTGRES_DB")
	SSL = "enable"
)
const (
	MAX_TRIES = 5
)

// Connect to the database
func init_db(s *Session) {
	dsn := fmt.Sprintf(
		"host=%s port=%s user=%s password=%s dbname=%s sslmode=%s", 
		HOST, PORT, UNAME, PSWRD,DNAME,SSL,
	)

	for range MAX_TRIES {
		db, err := sql.Open("postgres", dsn)
		if err != nil {
			s.Logger.Log_err("failed to open database", err)
			panic("")
		}

		if db.Stats().OpenConnections > 1 {
			// Optional: test connection
			if err := db.Ping(); err != nil {
				db.Close()
				s.Logger.Log_err("failed test ping on database", err)
				time.Sleep(time.Second*5)
				continue
			}
		} else {
			s.Logger.Log_err("DATABASE::Tried to open, no error, but no conns.", errors.New(""))
			time.Sleep(time.Second*5)
			continue
		}
		s.Db = &Database{DB: db}
		s.Logger.Log("Database connection initialized")
		return
	}


}

func (d *Database)Get_persisted_ips() (elems []nftables.SetElement) {

	const q = `SELECT ipv4 FROM NetworkState WHERE allowed = TRUE;`
	rows, err := d.Query(q)
	if err != nil { return elems }
	defer rows.Close()

	var rawIp string
	for rows.Next() {

        err = rows.Scan(&rawIp)
        if err != nil { return elems }

		elem := new(nftables.SetElement)
		elem.Key = net.ParseIP(rawIp).To4()

        elems = append(elems, *elem)
    }
	return elems
}
