package dpdk

import (
	"fmt"
	"strings"
	"testing"
)

func TestEal(t *testing.T) {
	ealArgsStr := "-l 0 -m 5 --no-pci --vdev=net_null0 --vdev=net_null1"
	ealArgs := strings.Split(ealArgsStr, " ")

	ret := EalInit(ealArgs)
	if ret != nil {
		fmt.Printf("EAL Error: %v\n", ret)
	}

	pool, err := PktMbufPoolCreate("test", 4096, 256, 0, RTE_MBUF_DEFAULT_BUF_SIZE, SOCKET_ID_ANY)
	if err != nil {
		t.Fatal("Can't create the MemPool:", err)
	}
	pool.Free()
	EalCleanup()
}
