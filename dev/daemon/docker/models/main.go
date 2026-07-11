package models

import (
	"daemon/docker/models/eth"
	"daemon/docker/utils"
	"encoding/json"
	"net"

	"github.com/docker/docker/client"
)

type Session struct {
	Client *client.Client
	Logger *utils.Logger

	// STATES
	Sz 	*SzState
	Pg 	*PostgresState
	Eth *eth.EthState
}

func (s *Session) CLI() *client.Client { return s.Client }
func (s *Session) LGR() *utils.Logger { return s.Logger }

func (s *Session) Handler(conn net.Conn, m *utils.Message) {
	switch m.Code {
		case utils.PRODUCTION, utils.NETWORK_CHANGE: 
			s.Production(&m.Payload) // They do same thing

		case utils.START_TEST: 
			s.Start_Test(conn, &m.Payload)

		case utils.RESTART_SZ: 
			s.Sz.Restart_Services(s, conn)
	}
}

func (s *Session) Start_Test(conn net.Conn, payload *json.RawMessage) {
	ok := utils.Start_Backend_Network(s.Client, s.Logger)
	if !ok { return }

	tp := new(utils.TestPayload)
	err := json.Unmarshal(*payload, tp)
	if err != nil {
		s.Logger.Log_err("UnmarshalTestPayload", err)
		return 
	}

	ok = s.Pg.Run_Test(s, tp.PgCount)
	if !ok { return }

	ok = s.Eth.Run_Test(s, tp.EthCount)
	if !ok { return }

	ok = s.Sz.Run_Test(s, tp.SzCount)
	if !ok { return }
}

func (s *Session) Production(payload *json.RawMessage) {
	ok := utils.Start_Backend_Network(s.Client, s.Logger)
	if !ok { return }

	pp := new(utils.ProductionPayload)
	err := json.Unmarshal(*payload, pp)
	if err != nil {
		s.Logger.Log_err("UnmarshalTestPayload", err)
		return 
	}

	ok = s.Pg.Run_Prod(s)
	if !ok { return }

	ok = s.Eth.Run_Prod(s, pp)
	if !ok { return }

	ok = s.Sz.Run_Prod(s)
	if !ok { return }
}

func New_Session(logger *utils.Logger) (*Session, error) {
	var err error
	s := new(Session)

	s.Logger = logger

	// Create a Docker client
	s.Client, err = client.NewClientWithOpts(
		client.FromEnv, client.WithAPIVersionNegotiation(),
	)
	if err != nil { 
		s.Logger.Log_err("NewClientWithOpts", err)
		return s, err
	}

	s.Sz, err = New_Sz(s.Client)
	if err != nil { 
		s.Logger.Log_err("NewSocialzState", err)
		return s, err
	}

	s.Pg, err = New_Pg(s.Client)
	if err != nil { 
		s.Logger.Log_err("NewPostgresState", err)
		return s, err
	}

	s.Eth, err = eth.New_Eth(s.Client)
	if err != nil { 
		s.Logger.Log_err("NewEthState", err)
		return s, err
	}
	return s, nil
}
