#! /bin/bash
#
# About: Install the Open vSwitch
#

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script with sudo."
    exit 1
fi

USER_HOME=$(getent passwd $SUDO_USER | cut -d: -f6)

OVS_DEV_VERSION="v2.11.0"
OVS_DPDK_VERSION="v2.11.0"
DPDK_VERSION="18.11"

function bootstrap() {
    # Remove installed OVS
    apt purge -y openvswitch-switch

    # Install depedencies
    apt update
    apt install -y gcc g++ make git
    apt install -y gcc-multilib
    apt install -y libc6-dev libnuma-dev
    apt install -y autoconf libssl-dev python2.7 python-six libtool
}

function add_systemd_services() {
    cat >/usr/lib/systemd/system/ovsdb-server.service <<EOL
[Unit]
Description=Open vSwitch Database Unit
After=syslog.target network-pre.target dpdk.service
Before=network.target networking.service
PartOf=openvswitch-switch.service
DefaultDependencies=no

[Service]
LimitNOFILE=1048576
Type=forking
Restart=on-failure
EnvironmentFile=-/etc/default/openvswitch
ExecStart=/usr/local/share/openvswitch/scripts/ovs-ctl \
--no-ovs-vswitchd --no-monitor --system-id=random \
start $OPTIONS
ExecStop=/usr/local/share/openvswitch/scripts/ovs-ctl --no-ovs-vswitchd stop
ExecReload=/usr/local/share/openvswitch/scripts/ovs-ctl --no-ovs-vswitchd \
--no-monitor restart $OPTIONS
RuntimeDirectory=openvswitch
RuntimeDirectoryMode=0755
EOL

    cat >/usr/lib/systemd/system/ovs-vswitchd.service <<EOL
[Unit]
Description=Open vSwitch Forwarding Unit
After=ovsdb-server.service network-pre.target systemd-udev-settle.service
Before=network.target networking.service
Requires=ovsdb-server.service
ReloadPropagatedFrom=ovsdb-server.service
AssertPathIsReadWrite=/usr/local/var/run/openvswitch/db.sock
PartOf=openvswitch-switch.service
DefaultDependencies=no

[Service]
LimitNOFILE=1048576
Type=forking
Restart=on-failure
Environment=HOME=/usr/local/var/run/openvswitch
EnvironmentFile=-/etc/default/openvswitch
ExecStart=ovs-vswitchd --pidfile --detach --log-file
ExecStop=/usr/local/share/openvswitch/scripts/ovs-ctl stop
TimeoutSec=300

[Install]
WantedBy=multi-user.target
EOL
}

function install_ovs_dpdk() {
    bootstrap
    # Install DPDK
    cd $USER_HOME || exit
    wget http://fast.dpdk.org/rel/"dpdk-$DPDK_VERSION.tar.xz"
    tar xf "dpdk-$DPDK_VERSION.tar.xz"
    export DPDK_DIR=$USER_HOME/"dpdk-$DPDK_VERSION"
    echo "export DPDK_DIR=${DPDK_DIR}" >>${USER_HOME}/.profile
    cd $DPDK_DIR || exit

    export DPDK_TARGET=x86_64-native-linuxapp-gcc
    export DPDK_BUILD=$DPDK_DIR/$DPDK_TARGET
    echo "export DPDK_BUILD=${DPDK_BUILD}" >>${USER_HOME}/.profile
    make install T=$DPDK_TARGET DESTDIR=install

    # Install OVS
    cd $USER_HOME || exit
    git clone https://github.com/openvswitch/ovs.git
    cd $USER_HOME/ovs || exit
    git checkout -b $OVS_DPDK_VERSION $OVS_DPDK_VERSION
    ./boot.sh
    autoreconf -i
    ./configure --with-dpdk=$DPDK_BUILD
    make CFLAGS='-O3 -march=native' -j "$(nproc)"
    make install

    echo "OVS_DPDK $OVS_DPDK_VERSION is installed."
}

function install_ovs_dev() {
    bootstrap
    cd $USER_HOME || exit
    git clone https://github.com/openvswitch/ovs.git
    cd $USER_HOME/ovs || exit
    git checkout -b $OVS_DEV_VERSION $OVS_DEV_VERSION
    ./boot.sh
    autoreconf -i
    ./configure
    make CFLAGS='-O3 -march=native' -j "$(nproc)"
    make install
    make modules_install
    modprobe openvswitch
    export PATH=$PATH:/usr/local/share/openvswitch/scripts
    ovs-ctl start --system-id=random
    echo "OVS $OVS_DEV_VERSION is installed."
}

function install_ovs_afxdp() {
    if pkg-config --libs libbpf >/dev/null 2>&1; then
        echo ""
    else
        echo "ERR: Can not find libbpf installed."
        exit 1
    fi
    bootstrap
    cd $USER_HOME || exit
    git clone https://github.com/openvswitch/ovs.git
    cd $USER_HOME/ovs || exit
    git reset --hard 9cfb1d0f7d65b52cc8f14d05c9e9ea8b64f90773
    ./boot.sh
    autoreconf -i
    ./configure --enable-afxdp

    cd $USER_HOME/ovs || exit
    make CFLAGS='-O3 -march=native' -j "$(nproc)"
    make install
    make modules_install
    modprobe openvswitch
    add_systemd_services
    systemctl daemon-reload
    # Start OVS after rebooting.
    systemctl enable ovs-vswitchd.service
    systemctl start ovs-vswitchd.service
}

if [[ "$1" == "dpdk" ]]; then
    echo "Build and install OVS with DPDK support."
    install_ovs_dpdk
elif [[ "$1" == "dev" ]]; then
    echo "Build and install OVS $OVS_DEV_VERSION for development."
    install_ovs_dev
elif [[ "$1" == "afxdp" ]]; then
    echo "Build and install OVS with afxdp support."
    install_ovs_afxdp
else
    echo "ERR: Invalid option!"
    printf "Usage:\n ./install_ovs.sh option {dev,dpdk,afxdp}\n"
fi
