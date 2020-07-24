#!/bin/bash

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script with sudo."
    exit 1
fi

SETUP=true

while :; do
    case $1 in
    -t)
        SETUP=false
        ;;
    *)
        break
        ;;
    esac
    shift
done

if [[ "$SETUP" == true ]]; then
    echo "Setup test environment."
    ovs-vsctl -- add-br br0 -- set Bridge br0 datapath_type=netdev
    ip netns add at_ns0
    ip link add p0 type veth peer name afxdp-p0
    ip link set p0 netns at_ns0
    ip link set dev afxdp-p0 up
    ovs-vsctl add-port br0 afxdp-p0 -- \
        set interface afxdp-p0 external-ids:iface-id="p0" type="afxdp"
    ip netns exec at_ns0 sh <<NS_EXEC_HEREDOC
    ip addr add "10.1.1.1/24" dev p0
    ip link set dev p0 up
NS_EXEC_HEREDOC
    ip netns add at_ns1
    ip link add p1 type veth peer name afxdp-p1
    ip link set p1 netns at_ns1
    ip link set dev afxdp-p1 up

    ovs-vsctl add-port br0 afxdp-p1 -- \
        set interface afxdp-p1 external-ids:iface-id="p1" type="afxdp"
    ip netns exec at_ns1 sh <<NS_EXEC_HEREDOC
ip addr add "10.1.1.2/24" dev p1
ip link set dev p1 up
NS_EXEC_HEREDOC

    ip netns exec at_ns0 ping -i .2 10.1.1.2

else
    echo "Teardown test environment."
    ip netns delete at_ns0
    ip netns delete at_ns1
    ovs-vsctl del-br br0
fi
