/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package firewall

import (
	"errors"
	"fmt"
	"log"
	"os"
	"time"
)

type Logger struct {
	f *os.File
}
const (
	LOG_PATH = "/var/log/socialz/net_worker.log"
)

// init_logger opens the log file and returns an initialized Logger.
func init_logger() *Logger {
	file, err := os.OpenFile(LOG_PATH, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		if errors.Is(err, os.ErrNotExist) {
			err = nil
			file, err = os.Create(LOG_PATH)
		}
		if err != nil {
			log.Fatalf("failed to open log file: %v", err)
		}
	}
	return &Logger{f: file}
}

// log writes a plain message with timestamp.
func (l *Logger) Log(msg string) {
	timestamp := time.Now().Format(time.RFC3339)
	entry := fmt.Sprintf("[%s] %s\n", timestamp, msg)
	if _, err := l.f.WriteString(entry); err != nil {
		log.Printf("logger write error: %v", err)
	}
}

// log_err writes an error message with context and timestamp.
func (l *Logger) Log_err(context string, err error) {
	timestamp := time.Now().Format(time.RFC3339)
	entry := fmt.Sprintf("[%s] ERROR: %s: %v\n", timestamp, context, err)
	if _, werr := l.f.WriteString(entry); werr != nil {
		log.Printf("logger write error: %v", werr)
	}
}
