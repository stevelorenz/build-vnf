#! /bin/bash
#
# About: Init Linux kernel IP forwarding
#

SUBNET_CIDR="10.0.12.0/24"
SRC_ADDR="10.0.12.4/32"
DST_ADDR="10.0.12.12/32"
IN_IFCE="eth1"
OUT_IFCE="eth2"

# Setup ingress and egress interface
ip link set $IN_IFCE up
ip link set $OUT_IFCE up
# Assign IP via DHCP
dhclient $IN_IFCE
dhclient $OUT_IFCE

# Remove duplicated routing items
ip route del $SUBNET_CIDR dev $IN_IFCE
ip route del $SUBNET_CIDR dev $OUT_IFCE

# Add static routes to src and dst
ip route add $SRC_ADDR dev $IN_IFCE
ip route add $DST_ADDR dev $OUT_IFCE

# Enable IP forwarding
echo 1 > /proc/sys/net/ipv4/ip_forward

# Turn off offloading
ethtool --offload $IN_IFCE rx off tx off
ethtool --offload $OUT_IFCE rx off tx off
