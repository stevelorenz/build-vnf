/*
 =====================================================================================
 *
 *       Filename:  xdp_udp_aes.c
 *    Description:  Encrypt UDP segememts with AES
 *
 *       Compiler:  llvm
 *          Email:  xianglinks@gmail.com
 *
 =====================================================================================
 */

#ifndef DEBUG
#define DBUG 0
#endif

#define KBUILD_MODNAME "xdp_udp_aes"

#ifndef AES_H
#include "../../../shared_lib/aes.h"
#endif

#ifndef XDP_UTIL_H
#include "../../../shared_lib/xdp_util.h"
#endif

/* Map for TX port */
BPF_DEVMAP(tx_port, 1);
/* Map for number of received packets */
BPF_PERCPU_ARRAY(udp_nb, long, 1);

uint16_t ingress_xdp_redirect(struct xdp_md* ctx) { return XDP_TX; }

uint16_t egress_xdp_tx(struct xdp_md* ctx) { return XDP_TX; }
