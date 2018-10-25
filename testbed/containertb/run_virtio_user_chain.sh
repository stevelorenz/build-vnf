#! /bin/bash
#
# About: Run two containers and connect them via virtio-user fast interface
#

TESTPMD_APP="dpdk_base"
TESTPMD_IMAGE="dpdk_base:v0.1"
PKTGEN_APP="dpdk_pktgen"
PKTGEN_IMAGE="dpdk_pktgen:v0.1"
BIND_IFCE="eth1"
WORKDIR=$(pwd)

function bootstrap() {
    echo "Setup OVS DPDK"
    bash /vagrant/script/setup_ovs_dpdk.sh
    echo "# Limit CPU usage of OVS-DPDK. By default, vhost-user port eating 100% of CPU."
    pgrep ovs-vswitchd | xargs -I {} sudo cpulimit -bq -p {} -l 30
}

function bind_interface() {
    echo "# Bind interface to DPDK"
    cd "$DPDK_DIR/usertools" || exit
    sudo ip link set $BIND_IFCE down
    sudo python ./dpdk-devbind.py --bind=igb_uio $BIND_IFCE
    python ./dpdk-devbind.py --status
}


function create_ovs_bridge() {
    echo "# Create OVS-DPDK bridge with vhost-user interface"
    sudo ovs-vsctl add-br br0 -- set bridge br0 datapath_type=netdev
    echo "# Create vhost user port"
    sudo ovs-vsctl add-port br0 vhost-user0 -- set Interface vhost-user0 type=dpdkvhostuser
    sudo ovs-vsctl add-port br0 vhost-user1 -- set Interface vhost-user1 type=dpdkvhostuser
    sudo ovs-vsctl add-port br0 vhost-user2 -- set Interface vhost-user2 type=dpdkvhostuser
    sudo ovs-vsctl add-port br0 vhost-user3 -- set Interface vhost-user3 type=dpdkvhostuser
    sudo ovs-vsctl show
}

function add_traffic_flow() {
    echo "# Add Flows into br0 to build the chain. TODO: This part SHOULD be done by SDN controller"
    sudo ovs-ofctl del-flows br0
    sudo ovs-ofctl add-flow br0 in_port=2,dl_type=0x800,idle_timeout=0,action=output:3
    sudo ovs-ofctl add-flow br0 in_port=3,dl_type=0x800,idle_timeout=0,action=output:2
    sudo ovs-ofctl add-flow br0 in_port=1,dl_type=0x800,idle_timeout=0,action=output:4
    sudo ovs-ofctl add-flow br0 in_port=4,dl_type=0x800,idle_timeout=0,action=output:1
    sudo ovs-ofctl dump-flows br0
}

function run_testpmd_app() {
    cd $WORKDIR || exit
    echo "Run $TESTPMD_APP App"
    sudo docker container stop "$TESTPMD_APP"
    sudo docker container rm "$TESTPMD_APP"
    sudo time docker run -dit --privileged \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v /usr/local/var/run/openvswitch:/var/run/openvswitch \
        -v /vagrant:/vagrant \
        --name "$TESTPMD_APP" "$TESTPMD_IMAGE" bash
    sudo docker cp ./run_testpmd.sh "$TESTPMD_APP":/root/

    sudo docker attach "$TESTPMD_APP"

    sudo docker container stop "$TESTPMD_APP"
    sudo docker container rm "$PKTGEN_APP"
}

function run_pktgen_app() {
    echo "# Run $PKTGEN_APP App."
    sudo docker container stop "$PKTGEN_APP"
    sudo docker container rm "$PKTGEN_APP"
    sudo time docker run -dit --privileged \
        -v /sys/bus/pci/drivers:/sys/bus/pci/drivers -v /sys/kernel/mm/hugepages:/sys/kernel/mm/hugepages -v /sys/devices/system/node:/sys/devices/system/node -v /dev:/dev \
        -v /vagrant:/vagrant \
        --name "$PKTGEN_APP" "$PKTGEN_IMAGE" bash

    sudo docker attach "$PKTGEN_APP"

    sudo docker container stop "$PKTGEN_APP"
    sudo docker container rm "$PKTGEN_APP"
}

if [[ "$1" == "app" ]]; then
    echo "Only run container apps."
    run_testpmd_app
else
    echo "Run full setups."
    bootstrap
    bind_interface
    create_ovs_bridge
    add_traffic_flow
    run_testpmd_app
fi
