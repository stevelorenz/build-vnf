#! /bin/bash
#
# setup_bm_test.sh
# About: Setup bare-mental tests
#        Bare-mental tests are used to evaluate the overhead of OpenStack's
#        virtualized network infrastructure
#        UDP sender and receiver are running in different network namespaces on
#        the same physical node. The network function program is running on
#        another physical node. Veth pairs and OVS are used to connect namespaces
#
#

#sudo apt update
#sudo apt install -y openvswitch-switch

LOCAL_PHY_IFCE="eth1"
LOCAL_PHY_IP="10.0.0.11/28"

SEND_IP="10.0.1.10/28"
RECV_IP="10.0.1.11/28"

# Create two namespaces and veths on one physical node
sudo ip netns add ns1
sudo ip netns add ns2

sudo ip link add veth0-ns1 type veth peer name veth0-root
sudo ip link set veth0-root up
sudo ip link set veth0-ns1 netns ns1
sudo ip netns exec ns1 ip link set veth0-ns1 up

sudo ip link add veth1-ns2 type veth peer name veth1-root
sudo ip link set veth1-root up
sudo ip link set veth1-ns2 netns ns2
sudo ip netns exec ns2 ip link set veth1-ns2 up

sudo ip netns exec ns1 ip addr add $SEND_IP dev veth0-ns1
sudo ip netns exec ns2 ip addr add $RECV_IP dev veth1-ns2

# Create a OVS to connect two namespaces and the phy ifce
sudo ovs-vsctl add-br br0
sudo ovs-vsctl add-port br0 veth0-root
sudo ovs-vsctl add-port br0 veth1-root
sudo ip addr flush $LOCAL_PHY_IFCE
sudo ovs-vsctl add-port br0 $LOCAL_PHY_IFCE
sudo ip addr add $LOCAL_PHY_IP dev br0
sudo ip link set br0 up

sudo ovs-vsctl show

echo "# Setup finished."
echo "    Run UDP sender in namespace ns1 with IP: $SEND_IP"
echo "    Run UDP receiver in namespace ns2 with IP: $RECV_IP"
