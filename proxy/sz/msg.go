package sz

/*
#include <sz/api/msgT.h>
#include <stdint.h>
*/
import "C"
import "unsafe"


type Msg struct {
	too 		uint8;
	from 		uint8;
	priority 	uint8;
	id 			uint16;
	code 		int;
	data 		*Vector;
};

type MsgBuffer struct {
	v *C.MsgBuffer
	s []*C.Msg
};

func New_buffer(b *C.MsgBuffer) *MsgBuffer {
	return &MsgBuffer{
		v: b,
		s: unsafe.Slice(b.msgs_, b.cap_),
	}
}

func (mb *MsgBuffer) consume_next() *C.Msg {
	if mb.v.cursor_ == mb.v.size_ { return nil; }
	m := mb.s[mb.v.cursor_]
	mb.v.cursor_++
	return m
}
func (mb *MsgBuffer) get_raw() *C.MsgBuffer { return mb.v; }
func (mb *MsgBuffer) remaining() int { return int(mb.v.size_ - mb.v.cursor_) }


