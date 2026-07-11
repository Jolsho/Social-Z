package eth

import (
	"daemon/docker/models/shared"
	"daemon/docker/utils"
	"os"

	"github.com/docker/docker/client"
)

const (
	NET_ID = "1313"

	HOST_ETH_MOUNT_BASE = "/opt/socialz/eth" 

	ETH_TAR = 	"/root/socialz"
	TAR_CONFY = ETH_TAR + "/config.yaml"
	TAR_GEN = 	ETH_TAR + "/genesis.json"
	TAR_JWT = 	ETH_TAR + "/jwt.hex"
	TAR_SPEC = 	ETH_TAR + "/chainspec.json"


	TEST_NET_ETH = utils.TEST_NET
	// Data Dir are not set because they can change...
	// keeping them off host machine simplifies cleanup
)

type EthState struct {
	exec *ExecutionState
	cons *ConsensusState
}

func New_Eth(cli *client.Client) (*EthState, error) {
	var err error
	s := new(EthState)

	s.exec, err = New_Execution(cli)
	if err != nil { return s, err }
	s.cons, err = New_Consensus(cli)
	return s, err
}

func (es *EthState) Run_Prod(s shared.SessionI, pp *utils.ProductionPayload) bool {
	if !Check_Network_Dir(pp.NetworkName) { return false }

	if !es.exec.Run_Prod(s, pp.NetworkName, pp.BootNode) { return false }

	return es.cons.Run_Prod(s, pp.NetworkName, pp.BootNode)
}

func (es *EthState)Run_Test(s shared.SessionI, counts ...int) bool {
	if !Check_Network_Dir(TEST_NET_ETH) { return false }

	if !es.exec.Run_Test(s) { return false }

	return es.cons.Run_Test(s, counts...)
}

func Check_Network_Dir(net_name string) bool {
	needed_files := []string{
		"config.yaml", "genesis.json", 
		"jwt.hex", "chainspec.json",
		"genesis.ssz",
	}

	for _, f := range needed_files {
		_, err := os.Stat(f)
		if err != nil && os.IsNotExist(err) { 
			// return false if "does not exist"
			return false
		}
	}
	return true
}
