package main

import (
	"daemon/docker/utils"
	"fmt"
	"os"
	"os/exec"
	"os/signal"
	"syscall"
)

const (
	PID_FILE_PATH = "/tmp/mydaemon.pid"
	STD_FDS_FILE = "/dev/null"

	LOG_PATH = "/var/log/socialz/daemon.log"
)

func launch_daemon() *utils.Logger {

	logger := utils.Init_logger(LOG_PATH)
	if os.Getenv("DAEMON_STAGE") == "" {
		// First fork
		cmd := exec.Command(os.Args[0], os.Args[1:]...)
		cmd.Env = append(os.Environ(), "DAEMON_STAGE=1")
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		cmd.Stdin = nil
		if err := cmd.Start(); err != nil {
			logger.Log_err("First fork failed: %v", err)
			os.Exit(1)
		}
		os.Exit(0)
	}

	if os.Getenv("DAEMON_STAGE") == "1" {
		// Create a new session to detach from terminal
		if _, err := syscall.Setsid(); err != nil {
			logger.Log_err("setsid failed: %v", err)
			os.Exit(1)
		}

		// Second fork
		cmd := exec.Command(os.Args[0], os.Args[1:]...)
		cmd.Env = append(os.Environ(), "DAEMON_STAGE=2")
		// Redirect all standard fds to /dev/null
		nullFile, _ := os.OpenFile(STD_FDS_FILE, os.O_RDWR, 0)
		cmd.Stdin = nullFile
		cmd.Stdout = nullFile
		cmd.Stderr = nullFile
		if err := cmd.Start(); err != nil {
			logger.Log_err("Second fork failed: %v", err)
			os.Exit(1)
		}
		os.Exit(0)
	}

	// Stage 2 - real daemon process
	// Change working dir and umask
	_ = os.Chdir("/")
	syscall.Umask(027)

	// Write PID file
	pid := os.Getpid()
	os.WriteFile(PID_FILE_PATH, fmt.Append([]byte{}, pid), 0644)
	logger.Log(fmt.Sprintf("Daemon started (pid=%d)", pid))

	// Setup signal handling
	sig := make(chan os.Signal, 1)
	signal.Notify(sig, syscall.SIGTERM, syscall.SIGINT)

	go func() {
		s := <-sig
		logger.Log(fmt.Sprintf("Received signal: %v, shutting down...", s))
		os.Remove(PID_FILE_PATH)
		os.Exit(0)
	}()

	return logger
}
