/*
 * udp_xor.hh
 * About: XOR UDP payload
 */

#ifndef UDP_XOR_HH
#define UDP_XOR_HH
#include <click/element.hh>
#include <click/glue.hh>
CLICK_DECLS

/**
 * @brief XOR the payload of a UDP segment
 */
class XORUDPPayload : public Element {
public:
    XORUDPPayload() CLICK_COLD;
    ~XORUDPPayload() CLICK_COLD;

    const char* class_name() const { return "XORUDPPayload"; }
    const char* port_count() const { return PORTS_1_1X2; }
    const char* processing() const { return PROCESSING_A_AH; }

    Packet* simple_action(Packet*);
};

#endif /* !UDP_XOR_HH */
