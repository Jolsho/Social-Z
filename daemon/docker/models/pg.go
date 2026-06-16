package models

import (
	"fmt"
	"os"
	"daemon/docker/models/shared"
	"daemon/docker/utils"

	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/mount"
	"github.com/docker/docker/api/types/network"
	"github.com/docker/docker/client"
)

const (
	pg_prefix = "postgres_node"
	pg_port = 5432
	postgres_image =  "postgres:16"

	cont_conf =  "/var/lib/postgresql/data/postgresql.conf" 
	host_conf =  "/opt/socialz/data/postgresql.conf"

	cont_db = "/var/lib/postgresql/data"
	host_db = "/opt/socialz/data"

	cont_init = "/docker-entrypoint-initdb.d"
    host_init = "/opt/socialz/data/init-scripts"
)

type PostgresState struct {
	*shared.State
}

func New_Pg(cli *client.Client) (*PostgresState, error) {
	s := new(PostgresState)
	s.State = shared.New_State(pg_prefix)
	return s, s.Fill_List_By_Name(cli,s.Prefix)
}

func (ps *PostgresState)Run_Prod(s shared.SessionI) bool {
	return ps.Run_Test(s, 1)
}

func (ps *PostgresState)Run_Test(s shared.SessionI, counts ...int) bool {
	if len(ps.Containers) > 0 {
		if err := ps.Stop_All(s); err != nil {
			s.LGR().Log_err("StopAll", err)
			return false
		}
	}

	config := &container.Config{
		Image:postgres_image,
		Env: Get_PG_Env_Vars(),
	}

	host_config := &container.HostConfig{
		AutoRemove: true,
		Mounts: []mount.Mount{
			{ 	// MAIN DB FILE
				Type: mount.TypeBind,
				Source: host_db,
				Target: cont_db,
			},

			{ 	// POSTGRES CONFIG
				Type: mount.TypeBind,
				Source: host_conf,
				Target: cont_conf,
				ReadOnly: true,
			},
			{ // Init scripts (run on first start)
                Type:     mount.TypeBind,
                Source:   host_init,
                Target:   cont_init,
                ReadOnly: true,
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
		return ps.Cleanup_On_Error(s,err,"CreateNewPg")
	}

	return true
}

func Get_PG_Env_Vars() []string {
	db_user := os.Getenv("POSTGRES_USER")
	db_pass := os.Getenv("POSTGRES_PASSWORD") 
	db_name := os.Getenv("POSTGRES_DB")
	db_host := os.Getenv("POSTGRES_HOST")
	db_port := os.Getenv("POSTGRES_PORT")

	return []string{
		fmt.Sprintf("POSTGRES_USER=%s",db_user),
		fmt.Sprintf("POSTGRES_PASSWORD=%s",db_pass),
		fmt.Sprintf("POSTGRES_DB=%s",db_name),
		fmt.Sprintf("POSTGRES_HOST=%s",db_host),
		fmt.Sprintf("POSTGRES_PORT=%s",db_port),
	}
}
