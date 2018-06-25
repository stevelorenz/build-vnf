/*
 =====================================================================================
 *
 *       Filename:  xdp_udp_aes.c
 *    Description:  Encrypt UDP segememts with XDP
 *
 *       Compiler:  llvm
 *          Email:  xianglinks@gmail.com
 *
 =====================================================================================
 */

#define KBUILD_MODNAME "xdp_udp_aes"
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if_vlan.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <uapi/linux/bpf.h>
