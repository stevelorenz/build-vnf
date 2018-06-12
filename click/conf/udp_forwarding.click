// udp_forwarding.click

// About: This configuration forwarding UDP segements from ingress port to egress port.
//        Filter UDP segements and also modify src,dst MAC addresses

// Email: xianglinks@gmail.com

// ---- Elements ---
AddressInfo(
    eth1 10.0.0.17 08:00:27:cb:53:a2,
    eth2 10.0.0.18 08:00:27:f1:51:f2,
    src 10.0.0.15 88:88:88:88:88:88,
    dst 10.0.0.16 99:99:99:99:99:99,
);
ingress :: FromDevice(eth1);
egress :: ToDevice(eth2);
is_ip :: Classifier(12/0800, -);
// Filter also source IP
is_udp :: IPClassifier(10.0.0.13/24 and udp, -);

// --- Connections ---
// TODO: Check if it is a looping frame

// Check if it is a IP datagram
ingress -> is_ip;

is_ip[1] -> Discard;

// Check if it is a UDP segement
is_ip[0]
    -> CheckIPHeader(14)
    -> is_udp;

is_udp[1] -> Discard;

is_udp[0]
    -> IPPrint
    // Change MAC to pass SFC
    -> EtherRewrite(src, dst)
    // Add a qeueu to connect push and pull ports
    -> FrontDropQueue(2000)
    -> egress;
