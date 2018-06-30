// udp_forwarding.click

// About: This configuration forwarding UDP segments from ingress port to egress port.
//        Filter UDP segments and also modify src,dst MAC addresses

// Email: xianglinks@gmail.com

// ---- Elements ---
AddressInfo(
    eth1 10.0.0.17 08:00:27:cb:53:a2,
    eth2 10.0.0.18 08:00:27:f1:51:f2,
    // source and destination addresses
    src 10.0.0.18 08:00:27:f1:51:f2,
    dst 10.0.0.13 08:00:27:1e:2d:b3,
);

ingress :: FromDevice(eth1);
egress :: ToDevice(eth2);
eth_ftr :: HostEtherFilter(dst, DROP_OWN true);
is_ip :: Classifier(12/0800, -);
// Filter also source IP
is_udp :: IPClassifier(10.0.0.13/24 and udp, -);
udp_ctr :: Counter();

// --- Connections ---
// Avoid looping frame
ingress -> eth_ftr
eth_ftr[0] -> is_ip;

// Check if it is a IP datagram
eth_ftr[1] -> is_ip;

is_ip[1] -> Discard;

// Check if it is a UDP segment
is_ip[0]
    -> CheckIPHeader(14)
    -> is_udp;

is_udp[1] -> Discard;

is_udp[0]
    -> IPPrint
    -> udp_ctr
    // Add a qeueu to connect push and pull ports
    -> FrontDropQueue(2000)
    // Change MAC to pass SFC
    -> EtherRewrite(src, dst)
    -> egress;
