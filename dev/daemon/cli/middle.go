/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package cli

import (
	"daemon/docker/models"
	"fmt"
	"os/exec"
)

func middleware(cmd string) error {
	switch cmd {
	case "restartf": return Rebuild_Front()
	case "restartb": return Rebuild_Back()
	default: return nil
	}
}

func Rebuild_Back() error {
	cmd := exec.Command("go", "build", "-o", "socialz", "main.go")
	cmd.Dir = models.HOST_SZ_SOURCE
	if err := cmd.Run(); err != nil { 
		fmt.Printf("ERROR:%s\n", err.Error())
		return err
	}
	return nil
}

func Rebuild_Front() error {
	// Rebuild frontend code
	front_dir := fmt.Sprintf("%s/front", models.HOST_SZ_SOURCE)
	cmd := exec.Command("npm", "run", "build")
	cmd.Dir = front_dir
	if err := cmd.Run(); err != nil { 
		fmt.Printf("ERROR:%s\n", err.Error())
		return err
	}

	// Remove previous build from server
	dist := fmt.Sprintf("%s/dist/", models.HOST_SZ_SOURCE)
	cmd = exec.Command("rm", "-rf", dist)
	if err := cmd.Run(); err != nil { 
		fmt.Printf("ERROR:%s\n", err.Error())
		return err
	}

	// move new build into server
	cmd = exec.Command("mv", "dist", models.HOST_SZ_SOURCE)
	if err := cmd.Run(); err != nil { 
		fmt.Printf("ERROR:%s\n", err.Error())
		return err
	}
	return nil
}
