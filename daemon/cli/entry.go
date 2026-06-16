package cli

import (
	"bufio"
	"daemon/docker"
	"daemon/docker/utils"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"strings"
)

func Run() {
	reader := bufio.NewReader(os.Stdin)

	conn, err :=  net.Dial("tcp", docker.DAEMON_SOCKET)
	if err != nil {
		panic(fmt.Errorf("NETDIAL::%s", err.Error()))
	}

	for {
		fmt.Print("> ")
		line, err := reader.ReadString('\n')
		if err != nil {
			if err == io.EOF {
				fmt.Println("Exiting...")
				return
			}
			fmt.Fprintf(os.Stderr, "Error reading command: %v\n", err)
			continue
		}

		args := strings.Fields(line)
		if len(args) == 0 { continue }

		err = handler(conn, args)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Handler error: %v\n", err)
			continue
		}
		defer conn.Close()

		// Pipe the response from the connection to stdout
		if _, err := io.Copy(os.Stdout, conn); err != nil {
			fmt.Fprintf(os.Stderr, "Error reading from connection: %v\n", err)
		}
	}
}

// Example handler function that returns a connection
func handler(conn net.Conn, args []string) (error) {
	var err error
	m := new(utils.Message)
	m.Code = command_to_code(args[0])
	if m.Code == -1 { 
		return errors.New("Invalid Code")
	}

	if m.Code > 69 {
		return handle_internal(m.Code, args)
	}

	err = middleware(args[0])
	if err != nil { return err }

	m.Payload, err = code_to_payload(m.Code, args)
	if err != nil { return err }


	m_bytes, err := json.Marshal(m)
	if err != nil { return err }


	_, err = conn.Write(m_bytes)

	return err
}

