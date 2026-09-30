/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package firewall

import (
	"encoding/json"
	"log"
	"net"

	"github.com/google/nftables"
	"github.com/google/nftables/expr"
)

const TABLE_N = "SOCIALZ"
const CHAIN_N = "SZ_INPUT"
const SET_N = "CITIZENS"

type newCitiM struct {
	New_ip string `json:"new_ip"`
	Old_ip string `json:"old_ip"`
}

func (s *Session)new_citizen(payload *json.RawMessage) {
	m := new(newCitiM)
	err := json.Unmarshal(*payload, &m)
	if err != nil {
		s.Logger.Log_err("NewCitizen:UNMARSHAL", err)
		return
	}
	set := make_citizen_set(nil)
	s.Conn.SetAddElements(set, parse_ips(m.New_ip))
	s.Conn.SetDeleteElements(set, parse_ips(m.Old_ip))
	err = s.Conn.Flush()
	if err != nil {
		s.Logger.Log_err("NewCitizen:FLUSH", err)
	}
}

type banCitiM struct {
	Ip string `json:"ip"`
}

func (s *Session)ban_citizen(payload *json.RawMessage) {
	m := new(banCitiM)
	err := json.Unmarshal(*payload, &m)
	if err != nil {
		s.Logger.Log_err("NewCitizen:UNMARSHAL", err)
		return
	}
	set := make_citizen_set(nil)
	s.Conn.SetDeleteElements(set, parse_ips(m.Ip))
	err = s.Conn.Flush()
	if err != nil {
		s.Logger.Log_err("NewCitizen:FLUSH", err)
	}
}

func (s *Session)unban_citizen(payload *json.RawMessage) {
	m := new(banCitiM)
	err := json.Unmarshal(*payload, &m)
	if err != nil {
		s.Logger.Log_err("NewCitizen:UNMARSHAL", err)
		return
	}
	set := make_citizen_set(nil)
	s.Conn.SetAddElements(set, parse_ips(m.Ip))
	err = s.Conn.Flush()
	if err != nil {
		s.Logger.Log_err("NewCitizen:FLUSH", err)
	}
}

func parse_ips(ips ...string) (set []nftables.SetElement) {
	set = make([]nftables.SetElement, 0, len(ips))
	for _, ip := range ips {
		set = append(set, *ip_to_element(ip))
	}
	return
}

func ip_to_element(ip string) *nftables.SetElement {
	e := new(nftables.SetElement)
	e.Key = net.ParseIP(ip).To4()
	return e
}

func make_citizen_table() *nftables.Table {
	return &nftables.Table{
		Family: nftables.TableFamilyINet,
		Name:   TABLE_N,
	}
}

func make_citizen_chain(table *nftables.Table) *nftables.Chain {
	if table == nil {
		table = make_citizen_table()
	}
	policy := nftables.ChainPolicyDrop
	return &nftables.Chain{
		Name:	  CHAIN_N,
		Table:    table,
		Type:     nftables.ChainTypeFilter,
		Hooknum:  nftables.ChainHookInput,
		Priority: nftables.ChainPriorityFilter,
		Policy:   &policy,
	}
}

func make_citizen_set(table *nftables.Table) *nftables.Set {
	if table == nil {
		table = make_citizen_table()
	}
	return &nftables.Set{
		Table:    table,
		Name:     SET_N,
		KeyType:  nftables.TypeIPAddr,
		DataType: nftables.TypeVerdict,
		Interval: false,
	}
}

func init_citizens(s *Session) {
	table := make_citizen_table()
	chain := make_citizen_chain(table)
	set := make_citizen_set(table)

	// Create a set of allowed IPv4 addresses
	// Add IPs to the set
	pers_ips := s.Db.Get_persisted_ips()

	err := s.Conn.AddSet(set, pers_ips)
	if err != nil {
		s.Logger.Log_err("NewCitizen:FLUSH", err)
		panic("")
	}

	// Rule: allow if source IP in set and dport 8080
	s.Conn.AddRule(&nftables.Rule{
		Table: table,
		Chain: chain,
		Exprs: []expr.Any{
			// payload: get daddr
			&expr.Payload{
				DestRegister: 1,
				Base:         expr.PayloadBaseNetworkHeader,
				Offset:       12, // IPv4 source address
				Len:          4,
			},
			&expr.Lookup{
				SourceRegister: 1,
				SetName:        set.Name,
				SetID:          set.ID,
			},
			// check dport == 8080
			&expr.Payload{
				DestRegister: 1,
				Base:         expr.PayloadBaseTransportHeader,
				Offset:       2, // TCP/UDP dest port
				Len:          2,
			},
			&expr.Cmp{
				Op:       expr.CmpOpEq,
				Register: 1,
				Data:     []byte{0x1f, 0x90}, // 8080 in hex
			},
			&expr.Verdict{
				Kind: expr.VerdictAccept,
			},
		},
	})

	// Default drop rule
	// Already enforced by chain policy, so no explicit rule needed

	if err := s.Conn.Flush(); err != nil {
		s.Logger.Log_err("SSH:FLUSH" , err)
		log.Fatalf("CITIZEN: %s", err)
	}
}
