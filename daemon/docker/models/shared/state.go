package shared

import (
	"context"
	"fmt"
	"daemon/docker/utils"
	"io"

	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/filters"
	"github.com/docker/docker/api/types/network"
	"github.com/docker/docker/client"
)

type SessionI interface {
	CLI() *client.Client
	LGR() *utils.Logger
}

type State struct {
	Prefix string
	Is_Testing bool
	Is_Production bool
	Containers []*StateContainer
}

func New_State(prefix string) *State {
	s := new(State)
	s.Prefix = prefix
	return s
}

func (s *State) Create_New(cli *client.Client, 
	config *container.Config, hc *container.HostConfig, 
	nc *network.NetworkingConfig,
) error { 

	new_name := fmt.Sprintf("%s%d",s.Prefix, len(s.Containers))
	res, err := cli.ContainerCreate(context.Background(), 
		config, hc, nc, nil, new_name,
	)
	if err != nil { return err }

	sum, err := utils.Get_summary_by_id(cli, res.ID)
	if err != nil { return err }
	c := new(StateContainer) 
	c.Summary = sum

	s.Containers = append(s.Containers, c)
	return nil
}

func (s *State)Fill_List_By_Name(cli *client.Client,name string) (error) {
	filterArgs := filters.NewArgs()
	filterArgs.Add("name", name)

	list, err :=  cli.ContainerList(
		context.Background(), container.ListOptions{
			Filters: filterArgs, All: true,
		}) 
	if err != nil { return err }

	containers := make([]*StateContainer, 0, len(list))
	for _,c := range list {
		sc := new(StateContainer)
		sc.Summary = &c
		containers = append(containers, sc)
	}

	s.Containers = containers

	return nil
}

func (s *State)Stop_All(se SessionI) error {
	ctx := context.Background()
	for _, c:= range s.Containers {
		err := se.CLI().ContainerStop(ctx, c.ID, container.StopOptions{})
		if err != nil { return err }
	}
	s.Containers = s.Containers[:0]
	return nil
}

func (st *State)Cleanup_On_Error(s SessionI, err error, msg string) bool {
    s.LGR().Log_err(msg, err)
    if len(st.Containers) > 0 {
        if err := st.Stop_All(s); err != nil {
            s.LGR().Log_err(msg+"StopAll", err)
        }
    }
    return false
}

type StateContainer struct {
	*container.Summary
	Process container.ExecInspect
}

func (c *StateContainer) Exec (s SessionI, cmd []string, wrkdir string, out io.Writer) error {
	ctx := context.Background()
	execConfig := container.ExecOptions{
		Cmd:          cmd,
		AttachStdout: true,
		AttachStderr: true,
		Tty:          false,
		WorkingDir:   wrkdir, // optional: set container working dir
	}

	resp, err := s.CLI().ContainerExecCreate(ctx, c.ID, execConfig)
	if err != nil {
		s.LGR().Log_err("ContainerExecCreate", err)
		return err 
	}

	//  Attach to the exec session
	attachResp, err := s.CLI().ContainerExecAttach(ctx, resp.ID, container.ExecAttachOptions{})
	if err != nil {
		s.LGR().Log_err("ContainerExecAttach", err)
		return err 
	}
	defer attachResp.Close()

	// COPY to out
	go func() {
		if out != nil {
			io.Copy(out, attachResp.Reader)
		}
	}()

	// Assign process in order to know PID/exit codes
	c.Process, err = s.CLI().ContainerExecInspect(ctx, resp.ID)
	if err != nil { s.LGR().Log_err("ContainerExecInspect", err) }
	return err
}
