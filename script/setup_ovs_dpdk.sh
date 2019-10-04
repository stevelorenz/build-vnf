#! /bin/sh
#
# setup_ovs_dpdk.sh
#
# About: Install and setup OVS with DPDK as Datapath
#        OVS uses DPDK library to operate entirely in userspace
# Distro: Ubuntu 16.04
#
#  MARK: Make sure it is a clean system
#        - OVS-DPDK is used for bare-mental tests

# Config DPDK
sudo sysctl -w vm.nr_hugepages=512

# Install and load kernel modules
if grep -Fxq "igb_uio" /etc/modules; then
    echo "* Kernel modules are already loaded."
else
    sudo modprobe uio
    sudo insmod "${DPDK_BUILD}/kmod/igb_uio.ko"
    sudo ln -sf "${DPDK_BUILD}/kmod/igb_uio.ko" "/lib/modules/$(uname -r)"
    sudo depmod -a
    echo "uio" | sudo tee -a /etc/modules
    echo "igb_uio" | sudo tee -a /etc/modules
fi

# Bind interface
# ${DPDK_DIR}/usertools/dpdk-devbind.py --status
# sudo ip link set eth2 down
# sudo ${DPDK_DIR}/usertools/dpdk-devbind.py --bind=igb_uio 0000:00:09.0
# ${DPDK_DIR}/usertools/dpdk-devbind.py --status

## Cleanup OVS db
sudo mkdir -p /usr/local/etc/openvswitch
sudo mkdir -p /usr/local/var/run/openvswitch
sudo rm /usr/local/etc/openvswitch/conf.db
sudo ovsdb-tool create /usr/local/etc/openvswitch/conf.db /usr/local/share/openvswitch/vswitch.ovsschema

# Start OVS except for ovs-vswitchd, which require config to enable DPDK
cd /usr/local/share/openvswitch/scripts || exit
# Start the ovs-db server
sudo ./ovs-ctl --no-ovs-vswitchd --system-id=random start
sudo mkdir -p /usr/local/etc/openvswitch
sudo ovs-vsctl --no-wait init
sudo ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
# MARK: Configure the amount of memory used in MB
sudo ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-socket-mem="128"
# Start ovs-vswitchd
sudo ./ovs-ctl --no-ovsdb-server --db-sock=/usr/local/var/run/openvswitch/db.sock start
echo "Validate OVS DPDK setup, should print true."
sudo ovs-vsctl get Open_vSwitch . dpdk_initialized
echo "Print versions"
sudo ovs-vswitchd --version
sudo ovs-vsctl show

echo ""
echo "OVS DPDK is configured."
echo "Connect network interfaces with ovs-vsctl."
