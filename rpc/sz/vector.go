package sz

/*
#include <sz/api/vec.h>
#include <stdint.h>
*/
import "C"
import (
	"sync"
	"unsafe"
)

type Vector struct {
	v *C.Vec;
	s []C.uchar;
}
func New_vector(v *C.Vec) *Vector {
	vv := Vector{
		v: v,
		s: unsafe.Slice(v.b, v.cap),
	}
	return &vv
}
func (v *Vector) get_raw() *C.Vec { return v.v }
func (v *Vector) size() int { return int(v.v.len); }
func (v *Vector) capacity() int { return int(v.v.cap); }
func (v *Vector) back(b *byte) { 
	l := int(v.v.len) - 1
	if l == -1 {
		b = nil
		return
	}
	*b = byte(v.s[l]) 
}
func (v *Vector) push_back(b byte) bool {
	if (v.v.len == v.v.cap) { return false }
	v.s[v.v.len] = C.uchar(b)
	v.v.len++
	return true
}
func (v *Vector) pop_back() (byte, bool) {
	l := int(v.v.len) - 1
	if l == -1 { return 0, false }
	v.v.len--
	return byte(v.s[l]), true
}



type BufferStore struct {
	mux 	*sync.Mutex;
	sizes 	[]int
	buckets [][]*Vector
}

const MAX_BUFFER_SIZE = 65536

func New_store() BufferStore {
	var bs BufferStore
	bs.sizes = append(bs.sizes, 256, 1024, 4096, 16384, MAX_BUFFER_SIZE)
	bs.buckets = make([][]*Vector, 0, len(bs.sizes))
	bs.mux = &sync.Mutex{}

	counts := []int{50, 30, 20, 10, 10}

	for i, s := range bs.sizes {
		for k := 0; k < counts[i]; k++ {
			bs.buckets[i] = append(bs.buckets[i], New_vector(C.new_vec(C.size_t(s))))
		}
	}

	return bs
}

func (bs *BufferStore) get_buffer(size int) *Vector {
	bs.mux.Lock();
	defer bs.mux.Unlock()
	for i, sz := range bs.sizes {
		if sz >= size {
			b := &bs.buckets[i]
			length := len(*b)
			if length == 0 { 
				return New_vector(C.new_vec(C.size_t(sz))) 
			}
			last := (*b)[length-1]
			*b = (*b)[:length-1]
			return last
		}
	}
	return nil
}

func (bs *BufferStore) put_buffer(v *Vector) {
	bs.mux.Lock();
	defer bs.mux.Unlock()
	for i, sz := range bs.sizes {
		bucket := &bs.buckets[i]
		if sz == v.capacity() && len(*bucket) < cap(*bucket){
			*bucket = append(*bucket, v)
			return
		}
	}
	C.free_vec(v.get_raw())
}
