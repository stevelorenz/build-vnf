package dpdk

/*
#cgo CFLAGS: -m64 -pthread -O3 -march=native -I/usr/local/include
#cgo LDFLAGS: -L/usr/local/lib/x86_64-linux-gnu -Wl,--as-needed -lrte_eal -lrte_mbuf -lrte_mempool

#include <stdlib.h>
#include <rte_config.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_memory.h>
*/
import "C"
import "unsafe"

const (
	SOCKET_ID_ANY              = int(C.SOCKET_ID_ANY)
	RTE_MBUF_PRIV_ALIGN        = uint(C.RTE_MBUF_PRIV_ALIGN)
	RTE_MBUF_DEFAULT_BUF_SIZE  = uint(C.RTE_MBUF_DEFAULT_BUF_SIZE)
	RTE_PKTMBUF_HEADROOM       = uint(C.RTE_PKTMBUF_HEADROOM)
	RTE_MEMPOOL_CACHE_MAX_SIZE = uint(C.RTE_MEMPOOL_CACHE_MAX_SIZE)
)

// MemPool represents a RTE mempool struct.
type MemPool C.struct_rte_mempool

// PktMbufPoolCreate creates and initializes a packet mbuf pool.
func PktMbufPoolCreate(name string, n, cacheSize, privSize, dataRoomSize uint, socketID int) (*MemPool, error) {
	cname := C.CString(name)
	defer C.free((unsafe.Pointer)(cname))
	pool := (*MemPool)(C.rte_pktmbuf_pool_create(cname, C.unsigned(n), C.unsigned(cacheSize),
		C.uint16_t(privSize), C.uint16_t(dataRoomSize), C.int(socketID)))
	if pool == nil {
		return nil, GetRteErrno()
	}
	return pool, nil
}

func (mp *MemPool) Free() {
	C.rte_mempool_free((*C.struct_rte_mempool)(mp))
}
