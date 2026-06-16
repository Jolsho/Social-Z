package models

import (
	"daemon/docker/models/eth"
	"daemon/docker/models/shared"
	"daemon/docker/utils"
	"fmt"
	"net"

	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/mount"
	"github.com/docker/docker/api/types/network"
	"github.com/docker/docker/client"
	"github.com/docker/go-connections/nat"
)

const (
	sz_prefix = "socialz_node"
	sz_image = "socialz:latest"

	sz_port = 6969
	sz_test_http = 8080

	HOST_SZ_SOURCE = "/opt/socialz/source/back"

	target_net_mount = target_mount+"/eth"

	target_mount = "/go/socialz"
	process_name = "./socialz"
)

type SzState struct { 
	*shared.State
}

// This just loads existing containers matching name prefix
// socialz nodes can be restarted because they dont require new state
func New_Sz(cli *client.Client) (*SzState, error) {
	s := new(SzState)
	s.State = shared.New_State(sz_prefix)
	return s, s.Fill_List_By_Name(cli, s.Prefix)
}

func (sz *SzState)Restart_Services(s shared.SessionI, out net.Conn) {
	for _, c:=range sz.Containers {
		cmd := []string{"kill", "-2", fmt.Sprintf("%d", c.Process.Pid)}
		err := c.Exec(s, cmd, "", out)
		if err != nil {
			s.LGR().Log_err("RestartSZKILL", err)
		} else {
			cmd = []string{process_name}
			err := c.Exec(s, cmd, target_mount, out)
			if err != nil {
				s.LGR().Log_err("RestartSZSTART", err)
			}
		}
	}
}

func (ps *SzState)Run_Test(s shared.SessionI, counts ...int) bool {
	if len(ps.Containers) > 0 {
		if err := ps.Stop_All(s); err != nil {
			s.LGR().Log_err("StopAll", err)
			return false
		}
	}
	count := 1
	if len(counts) > 0 {
		count = counts[0]
	}

	env_vars := Get_PG_Env_Vars()
	for i := range count {
		config := &container.Config{
			Image: sz_image,
			WorkingDir: target_mount,
			Env: env_vars,
			Cmd: []string{ process_name },
		}

		// expose http ports incrementally based on i for testing
		port := fmt.Sprintf("%d", sz_test_http+i)
		the_port_key := nat.Port(fmt.Sprintf("%s/tcp", port))

		host_config := &container.HostConfig{
			AutoRemove: true,
			Mounts: []mount.Mount{
				{ 	// Source and Bin
					Type: mount.TypeBind,
					Source: HOST_SZ_SOURCE,
					Target: target_mount,
				},
				{	// Sockets for other processes
					Type: mount.TypeBind,
					Source: utils.HOST_SOCKETS,
					Target: target_mount,
				},
				{	// Network directories
					Type: mount.TypeBind,
					Source: eth.HOST_ETH_MOUNT_BASE,
					Target: target_net_mount,
				},
			},
			PortBindings: nat.PortMap{ 
				the_port_key: []nat.PortBinding{
					{ 
						HostIP:"0.0.0.0", 
						HostPort:port, 
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
			s.LGR().Log_err("CreateNewSZ",err)
			if len(ps.Containers) > 0 {
				if err := ps.Stop_All(s); err != nil {
					s.LGR().Log_err("CreateNewSzStopAll", err)
					return false
				}
			}
			return false
		}
	}
	return true
}

func (sz *SzState) Run_Prod(s shared.SessionI) bool {
	if len(sz.Containers) > 0 {
		if err := sz.Stop_All(s); err != nil {
			s.LGR().Log_err("StopAll", err)
			return false
		}
	}

	config := &container.Config{
		Image: sz_image,
		Env: Get_PG_Env_Vars(),
		WorkingDir: target_mount,
		Cmd: []string{ process_name },
	}

	host_config := &container.HostConfig{
		AutoRemove: true,

		Mounts: []mount.Mount{
			{ 	// Source and Bin
				Type: mount.TypeBind,
				Source: HOST_SZ_SOURCE,
				Target: target_mount,
			},
			{	// Sockets for other processes
				Type: mount.TypeBind,
				Source: utils.HOST_SOCKETS,
				Target: target_mount,
			},
			{	// Network directories
				Type: mount.TypeBind,
				Source: eth.HOST_ETH_MOUNT_BASE,
				Target: target_net_mount,
			},
		},
		PortBindings: nat.PortMap{
			"443/tcp": []nat.PortBinding{
				{ 	// HTTPS
					HostIP: "0.0.0.0",
					HostPort: "443",
				},
			},
			"80/tcp": []nat.PortBinding{
				{ 	// HTTP
					HostIP: "0.0.0.0",
					HostPort: "80",
				},
			},
			"6969/tcp": []nat.PortBinding{
				{ 	// P2P
					HostIP: "0.0.0.0",
					HostPort: "6969",
				},
			},
		},
	}

	net_config := &network.NetworkingConfig{
		EndpointsConfig: map[string]*network.EndpointSettings{
			utils.BACKEND_NETWORK: {},
		},
	}

	err := sz.Create_New(s.CLI(), config, host_config, net_config)
	if err != nil {
		s.LGR().Log_err("CreateNewSZ",err)
		if len(sz.Containers) > 0 {
			if err := sz.Stop_All(s); err != nil {
				s.LGR().Log_err("CreateNewSzStopAll", err)
				return false
			}
		}
		return false
	}
	return true
}
