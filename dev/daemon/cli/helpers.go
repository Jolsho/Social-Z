package cli

import (
	"daemon/docker/utils"
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"strconv"
)

func command_to_code(cmd string) int {
	switch cmd {
	case "production": return utils.PRODUCTION
	case "change": return utils.NETWORK_CHANGE
	case "test": return utils.START_TEST
	case "restartf": return utils.RESTART_SZ
	case "restartb": return utils.RESTART_SZ

	case "h": return HELP
	case "q": return QUIT

	default: return -1
	}
}

func code_to_payload(code int, args []string) (json.RawMessage,error) {
	var payload any
	switch code {
	case utils.PRODUCTION, utils.NETWORK_CHANGE: 
		if len(args) != 3 {
			return nil, errors.New("Need args: [NetworkName] && [BootNode(enode)]")
		}
		payload = &utils.ProductionPayload{
			NetworkName: args[1],
			BootNode: args[2],
		}
		
	case utils.START_TEST: 
		if diff := 4 - len(args); diff > 0 {
			for ;diff > 0; diff-- {
				args = append(args, "1")
			}
		}
		PgCount, err := strconv.Atoi(args[1])
		if err != nil { return nil, err }

		SzCount, err := strconv.Atoi(args[2])
		if err != nil { return nil, err }

		EthCount, err := strconv.Atoi(args[3])
		if err != nil { return nil, err }

		payload = &utils.TestPayload{
			PgCount:PgCount, 
			SzCount:SzCount, 
			EthCount:EthCount, 
		}
		
	default:  payload = []string{}
	}

	return json.Marshal(payload)
}

func handle_internal(code int, args []string) error {
	switch code {
	case HELP: 
		if len(args) < 2 { 
			print_guide("all") 
		}
		print_guide(args[1])

	case QUIT: os.Exit(0)
	}

	return fmt.Errorf("invalid code %d", code)
}
const (
	HELP = 69
	QUIT = 70
)

func print_guide(cmd string) {

	switch cmd {
	case "production":
		println("[production] [NetworkName] [BootNode(enode)]")
		println("network name is the name of the directory")
		println("where all the information about network is stored")
		println("that dir is then mounted to Eth nodes.")
		println("Boot node is the enode url who shared the")
		println("network with you...can be blank if the founder.")

	case "change":
		println("[change] [NetworkName] [BootNode(enode)]")
		println("network name is the name of the directory")
		println("where all the information about network is stored")
		println("that dir is then mounted to Eth nodes.")
		println("Boot node is the enode url who shared the")
		println("network with you...can be blank if the founder.")

	case "test":
		println("[test] [DbCount] [SzCount] [EthCount]")
		println("each count is how many containers for each")
		println("if not specific all default to 1")

	case "restartb":
		println("[restart]")
		println("rebuilds backend source code")
		println("then restarts the socialz docker container")

	case "restartf":
		println("[restart]")
		println("rebuilds frontend source code")
		println("then restarts the socialz docker container")

	case "all":
		println("[production] [NetworkName] [BootNode(enode)]")
		println("[change] [NetworkName] [BootNode(enode)]")
		println("[test] [DbCount] [SzCount] [EthCount]")
		println("[restart]")
		println("[h] [^command^]")
		println("[q]")
	}
}
