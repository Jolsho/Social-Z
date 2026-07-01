package sz

/*
#cgo CFLAGS: -I/workspace/sz/node/install/include
#cgo LDFLAGS: -L/workspace/sz/node/install/lib -lsz

#include <sz/api/sz.h>
#include <sz/api/actor.h>
#include <sz/api/msgT.h>
#include <sz/db.h>
#include <sz/fs.h>
#include <sz/log.h>
#include <sz/p2p.h>
#include <stdint.h>
*/
import "C"
import "unsafe"


const EVENT_SIZE = 256
func start_sz() *C.Actor {
	szt := C.new_sz()

	conf := C.default_actor_config(C.ACTOR_DB, EVENT_SIZE)

	var db_conf C.DBConfig
	conf.id = C.ACTOR_DB
	db := C.start_db(C.new_actor(szt, &conf), &db_conf)

	var fs_conf C.FSConfig
	conf.id = C.ACTOR_FS
	fs := C.start_fs(C.new_actor(szt, &conf), &fs_conf)

	var log_conf C.LogConfig
	conf.id = C.ACTOR_LOG
	log := C.start_log(C.new_actor(szt, &conf), &log_conf)

	var p2p_conf C.P2PConfig
	conf.id = C.ACTOR_P2P
	p2p := C.start_p2p(C.new_actor(szt, &conf), &p2p_conf)

	conf.id = C.ACTOR_RPC
	actor := C.new_actor(szt, &conf)

	go func() {
		go C.run(szt);

		C.wait_on_actor_thread(db)
		C.wait_on_actor_thread(fs)
		C.wait_on_actor_thread(log)
		C.wait_on_actor_thread(p2p)
	}()

	return actor;
}

func Start_rpc(from_server chan Msg, to_server chan Msg, buffers *BufferStore) {

	actor := start_sz()

	events := C.new_event_buffer(EVENT_SIZE)
	in := New_buffer(C.new_msg_buffer(EVENT_SIZE));
	free_out := New_buffer(C.new_msg_buffer(EVENT_SIZE));

	for {
		C.poll_actor(actor, events, in.get_raw(), free_out.get_raw(), 50)
		stats := C.poll_telemetry(actor)
		if stats != nil {
			// LOG THE STATS
		}

		for 0 < in.remaining()   && 0 < free_out.remaining() {

			m_in := in.consume_next()
			if m_in == nil { continue; }

			// IF THIS IS JUST A RETURN MESSAGE WITH A BUFFER
			if (m_in.from == C.ACTOR_RPC && 
				m_in.data != nil && 
				m_in.priority == C.PRIORITY_CONT) {

				buffers.put_buffer(New_vector(m_in.data))
				m_in.data = nil

			} else {

				var m Msg;
				m.data = New_vector(m_in.data)
				m_in.data = nil
				m.code = int(m_in.code)
				m.id = uint16(m_in.code)
				m.too = uint8(m_in.too)
				m.from = uint8(m_in.from)
				to_server <- m
			}

		}


		msgs_to_handle := min(len(from_server), free_out.remaining())
		for 0 < msgs_to_handle {
			msgs_to_handle--

			msg := <- from_server

			fm := free_out.consume_next()
			if fm == nil { continue; }

			fm.id = C.ConnID(msg.id)
			fm.from = C.ACTOR_DB
			fm.priority = C.size_t(msg.priority)
			fm.too = C.Actors(msg.too)
			fm.is_wiped = false
			fm.code = C.int(msg.code)

			fm.data = msg.data.get_raw()
		}

		evs := unsafe.Slice(events.events, events.size)
		for i := 0; i < int(events.size); i++ {
			// TODO -- events??
			_ = &evs[i];

		}

		C.update_actor(actor, &in.v.cursor_, &free_out.v.cursor_)
	}
}
