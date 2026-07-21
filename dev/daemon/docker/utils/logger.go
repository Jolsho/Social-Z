/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package utils

import (
	"errors"
	"fmt"
	"log"
	"os"
	"syscall"
	"time"
)

type Logger struct {
	f *os.File
}

// Init_logger opens the log file and returns an initialized Logger.
func Init_logger(path string) *Logger {
	file, err := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			file, err = os.Create(path)
		}
		if err != nil {
			log.Fatalf("failed to open log file: %v", err)
		}
	}
	return &Logger{f: file}
}

// lockFile applies an exclusive advisory file lock using flock.
func (l *Logger) lockFile() error {
	return syscall.Flock(int(l.f.Fd()), syscall.LOCK_EX)
}

// unlockFile releases the file lock.
func (l *Logger) unlockFile() error {
	return syscall.Flock(int(l.f.Fd()), syscall.LOCK_UN)
}

// Log writes a plain message with timestamp.
func (l *Logger) Log(msg string) {
	// Lock the file for multi-process safety
	if err := l.lockFile(); err != nil {
		log.Printf("failed to acquire file lock: %v", err)
		return
	}

	timestamp := time.Now().Format(time.RFC3339)
	entry := fmt.Sprintf("[%s] %s\n", timestamp, msg)
	if _, err := l.f.WriteString(entry); err != nil {
		log.Printf("logger write error: %v", err)
	}

	if err := l.unlockFile(); err != nil {
		log.Printf("failed to release file lock: %v", err)
		return
	}
}

// Log_err writes an error message with context and timestamp.
func (l *Logger) Log_err(context string, err error) {
	// Lock the file for multi-process safety
	if err := l.lockFile(); err != nil {
		log.Printf("failed to acquire file lock: %v", err)
		return
	}

	timestamp := time.Now().Format(time.RFC3339)
	entry := fmt.Sprintf("[%s] ERROR: %s: %v\n", timestamp, context, err)
	if _, werr := l.f.WriteString(entry); werr != nil {
		log.Printf("logger write error: %v", werr)
	}

	if err := l.unlockFile(); err != nil {
		log.Printf("failed to release file lock: %v", err)
		return
	}
}

