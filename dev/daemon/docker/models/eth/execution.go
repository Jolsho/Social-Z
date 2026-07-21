/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

package eth

import (
	"daemon/docker/models/shared"
	"daemon/docker/utils"
	"fmt"

	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/mount"
	"github.com/docker/docker/api/types/network"
	"github.com/docker/docker/client"
	"github.com/docker/go-connections/nat"
)

const (
	exec_prefix = "eth_node"
	exec_auth_port = 8551
	exec_port = 30303
)

type ExecutionState struct {
	*shared.State
}

func New_Execution(cli *client.Client) (*ExecutionState, error) {
	s := new(ExecutionState)
	s.State = shared.New_State(exec_prefix)
	return s, s.Fill_List_By_Name(cli,s.Prefix)
}

// This just starts up a single execution engine
// because there is just no point in having more
// we run two consensus clients that connect to this thing
func (ps *ExecutionState)Run_Test(s shared.SessionI, counts ...int) bool {
	var err error

	if len(ps.Containers) > 0 {
		if err = ps.Stop_All(s); err != nil {
			s.LGR().Log_err("StopAll", err)
			return false
		}
	}

	count := 1
	if len(counts) > 0 { count = counts[0] }

	for i := range count {
		previous_enode := ""
		if i > 1 {
			prev_name := fmt.Sprintf("%s%d",ps.Prefix,i-1)
			previous_enode, err = utils.Get_ENR(prev_name, exec_port)
			if err != nil {
				return ps.Cleanup_On_Error(s,err,"GetENR")
			}
		}

		config := &container.Config{
			Image: "ghcr.io/paradigmxyz/reth",
			WorkingDir: "/root",
			Cmd: []string{
				"sh", "-c", "mkdir socialz && node",
				"--chain", TAR_SPEC,
				"--network-id", NET_ID,
				"--bootnodes", previous_enode, 

				"--authrpc.addr", "0.0.0.0",
				"--authrpc.port", fmt.Sprintf("%d",exec_auth_port),
				"--authrpc.jwtsecret", TAR_JWT,
				"--http", "--http.addr", "0.0.0.0", 
				"--http.port", "8545",
				"--http.api", "eth,net,web3",
			},
		}

		host_config := &container.HostConfig{
			AutoRemove: true,
			Mounts: []mount.Mount{
				{
					Type: mount.TypeBind,
					Source:HOST_ETH_MOUNT_BASE+"/"+TEST_NET_ETH,
					Target:ETH_TAR,
				},
			},
		}

		net_config := &network.NetworkingConfig{
			EndpointsConfig: map[string]*network.EndpointSettings{
				utils.BACKEND_NETWORK: {},
			},
		}

		err := ps.Create_New(s.CLI(), config, host_config, net_config)
		if err != nil {
			return ps.Cleanup_On_Error(s, err,"CreateNewExecution")
		}
	}
	return true
}

func (ps *ExecutionState)Run_Prod(s shared.SessionI, net_name, boot_node string) bool {
	if len(ps.Containers) > 0 {
		if err := ps.Stop_All(s); err != nil {
			s.LGR().Log_err("StopAll", err)
			return false
		}
	}

	config := &container.Config{
		Image: "ghcr.io/paradigmxyz/reth",
		WorkingDir: "/root",
		Cmd: []string{
			"sh", "-c", "mkdir socialz && node",
			"--chain", TAR_SPEC,
			"--network-id", NET_ID,
			"--bootnodes", boot_node,

			"--authrpc.addr", "0.0.0.0",
			"--authrpc.port", fmt.Sprintf("%d",exec_auth_port),
			"--authrpc.jwtsecret", TAR_JWT,
			"--http", "--http.addr", "0.0.0.0", 
			"--http.port", "8545",
			"--http.api", "eth,net,web3",
		},
	}

	host_config := &container.HostConfig{
		AutoRemove: true,
		Mounts: []mount.Mount{
			{
				Type: mount.TypeBind,
				Source:HOST_ETH_MOUNT_BASE+"/"+net_name,
				Target:ETH_TAR,
			},
		},
		PortBindings: nat.PortMap{
			"30303/tcp": []nat.PortBinding{
				{ 	// p2p tcp
					HostIP: "0.0.0.0",
					HostPort: "30303",
				},
			},
			"30303/udp": []nat.PortBinding{
				{ 	// p2p udp
					HostIP: "0.0.0.0",
					HostPort: "30303",
				},
			},
		},
	}

	net_config := &network.NetworkingConfig{
		EndpointsConfig: map[string]*network.EndpointSettings{
			utils.BACKEND_NETWORK: {},
		},
	}

	err := ps.Create_New(s.CLI(), config, host_config, net_config)
	if err != nil {
		return ps.Cleanup_On_Error(s, err,"CreateNewExecution")
	}

	return true
}
