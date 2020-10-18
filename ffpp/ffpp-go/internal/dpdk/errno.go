package dpdk

/*
#include <rte_errno.h>

int c_rte_errno() { return rte_errno; }
*/
import "C"
import "syscall"

// RteErrno represents a DPDK error
type RteErrno syscall.Errno

// GetRteErrno returns a the current DPDK error
func GetRteErrno() RteErrno {
	return RteErrno(C.c_rte_errno)
}

func (e RteErrno) Error() string {
	return C.GoString(C.rte_strerror(C.int(e)))
}
