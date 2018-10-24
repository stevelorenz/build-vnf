#! /bin/bash
#
# install_ovs_dpdk.sh
#
# About: Install OVS DPDK
#

OVS_VERSION="2.9.2"

# Remove installed OVS
sudo apt purge -y openvswitch-switch

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

echo "OVS_DPDK is installed."
