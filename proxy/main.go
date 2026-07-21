/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package main

import (
	"sz_proxy/server"
	"sz_proxy/sz"
)

func main() {

	to_rpc := make(chan sz.Msg, 100)
	to_server := make(chan sz.Msg, 100)

	buffers := sz.New_store();

	go sz.Start_rpc(to_rpc, to_server, &buffers)

	server.Start_server(to_server, to_rpc, &buffers)
}
