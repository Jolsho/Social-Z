/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package utils

import (
	"encoding/json"
)

const (
	HOST_SOCKETS = ROOT+"/sockets"

	PRODUCTION int = iota
	NETWORK_CHANGE
	START_TEST
	RESTART_SZ
)

type Message struct {
	Code int	`json:"header"`
	Payload json.RawMessage `json:"body"`
}

type TestPayload struct {
	PgCount int  `json:"pgcount"`
	SzCount int  `json:"szcount"`
	EthCount int `json:"ethcount"`
}

type ProductionPayload struct {
	NetworkName string  `json:"pgcount"`
	BootNode	string 	`json:"bootnode"`
}
