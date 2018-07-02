/*
 * About: XOR UDP payload
 */

// MUST include this firstly
#include <click/config.h>

#include "udpxorpayload.hh"
#include <click/error.hh>
#include <clicknet/ip.h>
#include <clicknet/udp.h>
CLICK_DECLS

#define XOR_OFFSET 16

UDPXORPayload::UDPXORPayload() {}

UDPXORPayload::~UDPXORPayload() {}

Packet* UDPXORPayload::simple_action(Packet* p_in)
{
    WritablePacket* p = p_in->uniqueify();
    click_ip* iph = p->ip_header();
    click_udp* udph = p->udp_header();
    int i;
    int payload_size;
    char* p_data;

    payload_size = ntohs(udph->uh_ulen) - 8 - XOR_OFFSET;
    p_data = (char*)udph + 8 + XOR_OFFSET;
    for (i = 0; i < payload_size; ++i) {
	*(p_data + i) = *(p_data + i) ^ 0x03;
    }
    return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(UDPXORPayload)
