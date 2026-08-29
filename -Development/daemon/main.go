/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package main

import (
	"daemon/cli"
	"daemon/docker"
	"daemon/router"
	"daemon/firewall"
	"errors"
	"fmt"
	"os"
	"strconv"
	"syscall"
)

// TODO -- maybe each of these .Run() commands returns a channel
// that channel is for shutdown signals
// if they are recieved we can just recall the run function
// unleass maybe there is some terminal error??

func main() {
	exists, err := check_PID_File()
	if exists && err == nil && os.Args[1] == "cli" {
		// run a cli for the running daemon
		cli.Run()

	} else if !exists && err == nil {

		// recursively executes this program and exits
		// until current process is daemon
		logger := launch_daemon()

		// Start the router/switch/WAP manager
		router.Run()

		// Start the docker daemon manager
		docker.Run(logger)

		// Start the nftables manager
		firewall.Run()

	} else if err == nil {
		println("Failed to start CLI, or Daemon.")
		println("There is already a daemon running,")
		println("OR")
		println("Add 'cli' to the end of the command.")
	} else {
		fmt.Printf("%s", err.Error())
	}
}

// checkPIDFile verifies if a daemon is already running
func check_PID_File() (exists bool, err error) {
	data, err := os.ReadFile(PID_FILE_PATH)
	if err != nil {
		if os.IsNotExist(err) {
			exists = false
			return // No PID file, no process
		}
		err = fmt.Errorf("error reading PID file: %w", err)
		return
	}

	pid, err := strconv.Atoi(string(data))
	if err != nil {
		err = fmt.Errorf("invalid PID in file: %w", err)
		return
	}

	// Check if process with this PID is running
	err = syscall.Kill(pid, 0)
	if err == nil {
		err = errors.New("daemon already running (pid active)")
		return
	}
	if err == syscall.ESRCH {
		// Process doesn't exist — stale PID file
		os.Remove(PID_FILE_PATH)
		exists = false
		return exists, nil
	}
	if err == syscall.EPERM {
		return true, errors.New("daemon already running (no permission to signal)")
	}

	return true, fmt.Errorf("error checking existing process: %w", err)
}
