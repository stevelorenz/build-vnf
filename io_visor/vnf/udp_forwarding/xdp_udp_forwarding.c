/*
 =====================================================================================
 *
 *       Filename:  xdp_udp_forwarding.c
 *    Description:  Forwarding UDP segments with XDP
 *
 *       Compiler:  llvm
 *          Email:  xianglinks@gmail.com
 *
 =====================================================================================
 */

/*
 * MARK:
 *       - For any bytes modifications, you MUST make sure the bytes you want to
 *       read are within the packet's range BEFORE reading them! Otherwise the
 *       compiler(BPF verifier) throws Permission denied (Invalid access to the
 *       packet).
 * */

#define KBUILD_MODNAME "xdp_udp_forwarding"
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/if_vlan.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <uapi/linux/bpf.h>

enum OPT_STATE { OPT_SUC = 10, OPT_FAIL = 0 };

enum ACTION { DROP = 0, BOUNCE, REDIRECT };
typedef enum ACTION ACTION;

/* Map for TX port */
BPF_DEVMAP(tx_port, 1);
/* Map for number of received packets */
BPF_PERCPU_ARRAY(udp_nb, long, 1);

/* Functions for frame filtering */
static inline uint16_t parse_eth(void* data, uint64_t nh_off, void* data_end)
{
        struct ethhdr* eth = data + nh_off;
        if (data + nh_off > data_end) {
                return OPT_FAIL;
        }
        return eth->h_proto;
}

static inline uint16_t parse_ipv4(void* data, uint64_t nh_off, void* data_end)
{
        struct iphdr* iph = data + nh_off;

        if ((void*)&iph[1] > data_end) {
                return OPT_FAIL;
        }
        return iph->protocol;
}

/* Functions for header modification */
static inline uint8_t* ether_aton(const uint8_t* asc)
{
        uint8_t ether_addr[ETH_ALEN];
        return ether_addr;
}

static inline uint8_t rewrite_mac(void* data, uint64_t nh_off, void* data_end)
{
        /*struct ethhdr {*/
        /*unsigned char	h_dest[ETH_ALEN];	[> destination eth addr	<]*/
        /*unsigned char	h_source[ETH_ALEN];	[> source ether addr	<]*/
        /*__be16		h_proto;		[> packet type ID field
         * <]*/
        /*} __attribute__((packed));*/
        struct ethhdr* eth = data + nh_off;
        if (eth + sizeof(struct ethhdr) > data_end) {
                return OPT_FAIL;
        }
        /* MARK: To be used source and destination are hard coded here */
        /*08:00:27:d6:69:61*/
        uint8_t src_mac[ETH_ALEN] = { 0x08, 0x00, 0x27, 0xd6, 0x69, 0x61 };
        /*08:00:27:e1:f1:7d*/
        uint8_t dst_mac[ETH_ALEN] = { 0x08, 0x00, 0x27, 0xe1, 0xf1, 0x7d };
        __builtin_memcpy(eth->h_source, src_mac, ETH_ALEN);
        __builtin_memcpy(eth->h_dest, dst_mac, ETH_ALEN);
        return OPT_SUC;
}

static inline uint8_t swap_mac(void* data, uint64_t nh_off, void* data_end)
{
        struct ethhdr* eth = data + nh_off;
        if (eth + sizeof(struct ethhdr) > data_end) {
                return XDP_DROP;
        }
        uint8_t tmp[ETH_ALEN];
        __builtin_memcpy(tmp, eth->h_source, ETH_ALEN);
        __builtin_memcpy(eth->h_source, eth->h_dest, ETH_ALEN);
        __builtin_memcpy(eth->h_dest, tmp, ETH_ALEN);
}

/* UDP forwarding functions */
uint16_t ingress_xdp_redirect(struct xdp_md* ctx)
{
        void* data_end = (void*)(long)ctx->data_end;
        void* data = (void*)(long)ctx->data;

        long* value;
        uint16_t h_proto;
        /*ACTION action = BOUNCE;*/
        ACTION action = REDIRECT;
        uint64_t nh_off = 0;
        uint32_t key = 0;

        /* TODO: Filter received Ethernet frame -> Only handle UDP segments */

        value = udp_nb.lookup(&key);
        if (value) {
                *value += 1;
        }
        nh_off = 0;

        if (action == BOUNCE) {
                if (rewrite_mac(data, nh_off, data_end) == OPT_FAIL) {
                        return XDP_DROP;
                }
                return XDP_TX;
        }

        if (action == REDIRECT) {
                if (rewrite_mac(data, nh_off, data_end) == OPT_FAIL) {
                        return XDP_DROP;
                }
                return tx_port.redirect_map(0, 0);
        }

        return XDP_DROP;
}

uint16_t egress_xdp_tx(struct xdp_md* ctx) { return XDP_TX; }
