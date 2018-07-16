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

OVS_VERSION="2.9.2"

# Install depedencies
sudo apt update
sudo apt install -y gcc g++ make git
sudo apt install -y gcc-multilib
sudo apt install -y libc6-dev libnuma-dev
sudo apt install -y autoconf libssl-dev python2.7 python-six libtool

# Install DPDK
cd $HOME || exit
wget http://fast.dpdk.org/rel/dpdk-17.11.3.tar.xz
tar xf dpdk-17.11.3.tar.xz
export DPDK_DIR=$HOME/dpdk-stable-17.11.3
echo "export DPDK_DIR=${DPDK_DIR}" >> ${HOME}/.profile
cd $DPDK_DIR || exit

export DPDK_TARGET=x86_64-native-linuxapp-gcc
export DPDK_BUILD=$DPDK_DIR/$DPDK_TARGET
echo "export DPDK_BUILD=${DPDK_BUILD}" >> ${HOME}/.profile
make install T=$DPDK_TARGET DESTDIR=install

# Install OVS
cd $HOME || exit
git clone https://github.com/openvswitch/ovs.git
cd $HOME/ovs || exit
git checkout -b $OVS_VERSION $OVS_VERSION
./boot.sh
autoreconf -i
./configure --with-dpdk=$DPDK_BUILD
make CFLAGS='-O3 -march=native' -j 2
sudo make install

# Config DPDK
sudo sysctl -w vm.nr_hugepages=512

# Install and load kernel modules
sudo modprobe uio
sudo insmod ${DPDK_BUILD}/kmod/igb_uio.ko
sudo ln -sf ${DPDK_BUILD}/kmod/igb_uio.ko /lib/modules/`uname -r`
sudo depmod -a
echo "uio" | sudo tee -a /etc/modules
echo "igb_uio" | sudo tee -a /etc/modules

# Bind interface
${DPDK_DIR}/usertools/dpdk-devbind.py --status
sudo ip link set eth2 down
sudo ${DPDK_DIR}/usertools/dpdk-devbind.py --bind=igb_uio 0000:00:09.0
${DPDK_DIR}/usertools/dpdk-devbind.py --status

## Cleanup OVS db
sudo mkdir -p /usr/local/etc/openvswitch
sudo mkdir -p /usr/local/var/run/openvswitch
sudo rm /usr/local/etc/openvswitch/conf.db
sudo ovsdb-tool create /usr/local/etc/openvswitch/conf.db  /usr/local/share/openvswitch/vswitch.ovsschema

# Start OVS except for ovs-vswitchd, which require config to enable DPDK
cd /usr/local/share/openvswitch/scripts || exit
# Start the ovs-db server
sudo ./ovs-ctl --no-ovs-vswitchd start
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

echo ""
echo "OVS-DPDK is installed and configured."
