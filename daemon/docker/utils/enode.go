package utils

import (
	"crypto/rand"
	"errors"
	"net"
	"os"

	"github.com/ethereum/go-ethereum/crypto"
	"github.com/ethereum/go-ethereum/p2p/enode"
)

const (
	ROOT = "/opt/socialz"

	TEST_NET_KEYS = ROOT+"/eth/"+TEST_NET+"/keys"
	TEST_NET = "test_net"
)

func Get_ENR(name string, port int) (string,error) {
	key , err := Get_Node_Key(name)
	if err != nil { return "", err }

	priv, err := crypto.ToECDSA(key)
	if err != nil { return "", err }

    // Create an ENR with IP and ports
	ip, err := Get_Ip(name)
	if err != nil { return "", err }

	nodeUrl := enode.NewV4(&priv.PublicKey, ip, port, port).URLv4()

	return nodeUrl, nil 
}

func Get_Ip(container_name string) (net.IP, error) {
    ips, err := net.LookupIP(container_name)
    if err != nil {
		return nil ,err
    }
    // Take the first IP (IPv4 preferably)
    var ip net.IP
    for _, candidate := range ips {
        if candidate.To4() != nil {
            ip = candidate
            break
        }
    }
    if ip == nil {
        return nil ,errors.New("No IPv4 address found for container")
    }
	return ip, nil
}

func Get_Node_Key(name string) ([]byte, error) {
	key := make([]byte, 32)
	file, err := os.Open(TEST_NET_KEYS+name+".txt")
	if os.IsNotExist(err) {
		// create the file and insert a key
		file, err = os.Create(TEST_NET_KEYS+name+".txt")
		if err != nil { return nil, err }

		defer file.Close()

		_, err = rand.Read(key)
		if err != nil { return nil, err }

		_, err = file.Write(key)
		if err != nil { return nil, err }

	} else {
		_,err = file.Read(key)
		if err != nil { return nil, err }
	}
	return key, nil

}
