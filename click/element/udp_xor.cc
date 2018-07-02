/*
 * udp_xor.cc
 * About: XOR UDP payload
 *
 */

#include "udp_xor.hh"

#include <click/args.hh>
#include <click/config.h>
#include <click/error.hh>
#include <click/glue.hh>
#include <click/router.hh>
#include <clicknet/ip.h>
#include <clicknet/udp.h>

CLICK_DECLS

XORUDPPayload::XORUDPPayload() {}
XORUDPPayload::~XORUDPPayload() {}

int XORUDPPayload::configure(Vector<String>& conf, ErrorHandler* errh)
{
    _data_offset = -1;
    if (Args(conf, this, errh).read_p("OFFSET", _data_offset).complete() < 0)
	return -1;
}

Packet* XORUDPPayload::simple_action(Packet* p) {}

CLICK_ENDDECLS
