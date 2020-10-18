package dpdk

/*
#cgo CFLAGS: -m64 -pthread -O3 -march=native -I/usr/local/include
#cgo LDFLAGS: -L/usr/local/lib/x86_64-linux-gnu -Wl,--as-needed -lrte_eal

#include <stdlib.h>
#include <rte_config.h>
#include <rte_eal.h>
*/
import "C"
import "unsafe"

// EalInit initializes the EAL environment.
func EalInit(argv []string) error {
	cargv := make([]*C.char, len(argv))
	for i, arg := range argv {
		cargv[i] = C.CString(arg)
		defer C.free(unsafe.Pointer(cargv[i]))
	}
	if int(C.rte_eal_init(C.int(len(cargv)), (**C.char)(&cargv[0]))) == -1 {
		return GetRteErrno()
	}
	return nil
}

// EalCleanup cleanup the EAL environment.
func EalCleanup() {
	C.rte_eal_cleanup()
}
