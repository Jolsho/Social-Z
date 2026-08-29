/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package firewall

import (
	"log"
	"net"

	"github.com/google/nftables"
	"github.com/google/nftables/expr"
)

const (
	MY_IP = "192.168.137.1"
)

func init_ssh(s *Session) {
	// Define a new inet table
	table := make_citizen_table()
	s.Conn.AddTable(table)

	// Create a new chain with input hook
	chain := make_citizen_chain(table)
	s.Conn.AddChain(chain)

	// Rule: allow if source IP == 192.168.137.1 and dport == 22
	s.Conn.AddRule(&nftables.Rule{
		Table: table,
		Chain: make_citizen_chain(table),
		Exprs: []expr.Any{
			// Match source IP
			&expr.Payload{
				DestRegister: 1,
				Base:         expr.PayloadBaseNetworkHeader,
				Offset:       12,
				Len:          4,
			},
			&expr.Cmp{
				Register: 1,
				Op:       expr.CmpOpEq,
				Data:     net.ParseIP(MY_IP).To4(),
			},
			// Match destination port 22
			&expr.Payload{
				DestRegister: 1,
				Base:         expr.PayloadBaseTransportHeader,
				Offset:       2,
				Len:          2,
			},
			&expr.Cmp{
				Register: 1,
				Op:       expr.CmpOpEq,
				Data:     []byte{0x00, 0x16}, // 22 in hex
			},
			&expr.Verdict{
				Kind: expr.VerdictAccept,
			},
		},
	})
	if err := s.Conn.Flush(); err != nil {
		s.Logger.Log_err("SSH:FLUSH" , err)
		log.Fatalf("ssh: %s", err)
	}
}
