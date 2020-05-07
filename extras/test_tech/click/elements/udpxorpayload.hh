#ifndef CLICK_UDPXORPAYLOAD_HH
#define CLICK_UDPXORPAYLOAD_HH

#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

/*
 */

class UDPXORPayload : public Element {
public:
    UDPXORPayload() CLICK_COLD;
    ~UDPXORPayload() CLICK_COLD;

    const char* class_name() const { return "UDPXORPayload"; }
    const char* port_count() const { return PORTS_1_1; }

    Packet* simple_action(Packet*);
};

CLICK_ENDDECLS
#endif
