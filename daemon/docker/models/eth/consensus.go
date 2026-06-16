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
	consensus_prefix = "eth_consensus"
	consensus_port = 9000
)

type ConsensusState struct {
	*shared.State
}

func New_Consensus(cli *client.Client) (*ConsensusState, error) {
	s := new(ConsensusState)
	s.State = shared.New_State(consensus_prefix)
	return s, s.Fill_List_By_Name(cli,s.Prefix)
}

func (ps *ConsensusState)Run_Test(s shared.SessionI, counts ...int) bool {
	var err error

	if len(ps.Containers) > 0 {
		if err = ps.Stop_All(s); err != nil {
			s.LGR().Log_err("StopAll", err)
			return false
		}
	}
	count := 1
	if len(counts) > 0 {
		count = counts[0]
	}

	for i := range count {

		exec_end := fmt.Sprintf("http://%s%d:%d",exec_prefix, i, exec_auth_port+i)
		name := fmt.Sprintf("%s%d",ps.Prefix,i)

		if _, err = utils.Get_Node_Key(name); err != nil {
			return ps.Cleanup_On_Error(s, err, "GetNodeKey")
		}

		previous_enr := ""
		if i > 1 {
			prev_name := fmt.Sprintf("%s%d",ps.Prefix,i-1)
			previous_enr, err = utils.Get_ENR(prev_name, consensus_port)
			if err != nil {
				return ps.Cleanup_On_Error(s,err,"GetENR")
			}
		}

		config := &container.Config{
			Image: "gcr.io/offchainlabs/prysm/beacon-chain:stable",
			Cmd: []string{ "prysm", "beacon-node",
				"--p2p-host-ip", name,
				"--p2p-priv-key", fmt.Sprintf(ETH_TAR+"/%s.txt",name),
				"--bootstrap-node", previous_enr,

				"--chain-config-file", TAR_CONFY,
				"--genesis-state", TAR_GEN,
				"--execution-endpoint", exec_end,
				"--execution-jwt", TAR_JWT,
			},
		}

		host_config := &container.HostConfig{
			AutoRemove: true,
			Mounts: []mount.Mount{
				{
					Type: mount.TypeBind,
					Source:HOST_ETH_MOUNT_BASE+"/"+TEST_NET_ETH,
					Target: ETH_TAR,
				},
			},
		}

		net_config := &network.NetworkingConfig{
			EndpointsConfig: map[string]*network.EndpointSettings{
				utils.BACKEND_NETWORK: {},
			},
		}

		err = ps.Create_New(s.CLI(), config, host_config, net_config)
		if err != nil {
			return ps.Cleanup_On_Error(s,err,"CreateNewSZ")
		}
	}
	return true
}

func (ps *ConsensusState)Run_Prod(s shared.SessionI, net_name, boot_node string) bool {

	if len(ps.Containers) > 0 {
		if err := ps.Stop_All(s); err != nil {
			s.LGR().Log_err("StopAll", err)
			return false
		}
	}
	exec_end := fmt.Sprintf("http://%s:%d",exec_prefix, exec_auth_port)
	config := &container.Config{
		Image: "gcr.io/offchainlabs/prysm/beacon-chain:stable",
		Cmd: []string{ "prysm", "beacon-node",
			"--bootstrap-node", boot_node,
			"--chain-config-file", TAR_CONFY,
			"--genesis-state", TAR_GEN,
			"--execution-endpoint", exec_end,
			"--execution-jwt", TAR_JWT,
		},
	}

	host_config := &container.HostConfig{
		AutoRemove: true,
		Mounts: []mount.Mount{
			{
				Type: mount.TypeBind,
				Source:HOST_ETH_MOUNT_BASE+"/"+net_name,
				Target: ETH_TAR,
			},
		},
		PortBindings: nat.PortMap{
			"9000/tcp": []nat.PortBinding{
				{
					HostIP: "0.0.0.0",
					HostPort: "9000",
				},
			},
			"9000/udp": []nat.PortBinding{
				{
					HostIP: "0.0.0.0",
					HostPort: "9000",
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
		return ps.Cleanup_On_Error(s,err,"CreateNewSZ")
	}
	return true
}
