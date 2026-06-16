package utils

import (
	"fmt"
	"context"

	"github.com/docker/docker/api/types/network"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/filters"
	"github.com/docker/docker/client"
)

func Get_summary_by_id(cli *client.Client, id string) (*container.Summary, error) {
    filterArgs := filters.NewArgs()
    filterArgs.Add("id", id)

    containers, err := cli.ContainerList(
        context.Background(),
        container.ListOptions{
            All:     true,
            Filters: filterArgs,
        },
    )
    if err != nil {
        return nil, err
    }

    if len(containers) == 0 {
        return nil, fmt.Errorf("container %s not found", id)
    }

    return &containers[0], nil
}

const (
	BACKEND_NETWORK = "backend"
)

func Start_Backend_Network(cli *client.Client, logg *Logger) bool {
	ctx := context.Background()

	// Check if BACKEND network exists
	networks, err := cli.NetworkList(ctx, network.ListOptions{})
	if err != nil {
		logg.Log_err("ListNetworks", err)
		return false
	}

	for _, n := range networks {
		if n.Name == BACKEND_NETWORK{
			return true
		}
	}

	// Create the bridge network if it doesn't exist
	_, err = cli.NetworkCreate(ctx,BACKEND_NETWORK, network.CreateOptions{
		Driver: "bridge",
	})
	if err != nil {
		logg.Log_err("CreateBackendNetwork", err)
		return false
	}
	return true
}
