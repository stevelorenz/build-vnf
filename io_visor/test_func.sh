#! /bin/bash
#
# About: Test the functionality of VNFs
#  MARK:
#        - Add static ARPs to avoid ARP requests

if [[ $1 = '-ir'  ]]; then
        # Init the receiver
        echo "Init the receiver"
        sudo arp -s 10.0.0.13 08:00:27:1e:2d:b3
        sudo arp -s 10.0.0.14 08:00:27:e1:f1:7d
        sudo arp -n
elif [[ $1 = '-is' ]]; then
        echo "Init the sender"
        sudo arp -s 10.0.0.15 08:00:27:d4:93:35
        sudo arp -s 10.0.0.16 08:00:27:d6:69:61
        sudo arp -n
elif [[ $1 = '-rs' ]]; then
        echo "Run sender to send UDP segments to IP: 10.0.0.13"
        python3 ../delay_timer/udp_owd_timer.py -c 10.0.0.15:6666 -n 3 --ipd 1 --payload_size 256 --log DEBUG
elif [[ $1 = '-ms' ]]; then
        echo "Monitor sender interface with tcpdump"
        sudo tcpdump -i eth2 -e -vv udp -X
elif [[ $1 = '-msc' ]]; then
        echo "Monitor sender interface with tcpdump, dump pcap file ./test.pcap"
        sudo tcpdump -i eth2 -e -vv udp -X -w ./test.pcap
else
        echo "Unknown option!"
fi
