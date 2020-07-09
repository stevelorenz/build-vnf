#! /usr/bin/env bash
#
# About: Setup pktgen, vnf and power-manager and loads the XDP hook

IFACE="eno2"
SETUP=true
PM=true
BASE_DIR=$(pwd)
KERN_DIR="../kern/xdp_time/"

# Argparse for setup or teardown and interface
# Maybe options to just start certain things etc.
while :; do
    case $1 in
    -t)
        SETUP=false
        ;;
    -i)
        if [ "$2" ]; then
            IFACE="$2"
            shift
        fi
        ;;
    -p)
        PM=false
        ;;
    *)
        break
        ;;
    esac
    shift
done

if [ "$SETUP" = true ]; then
    echo "* Setup test environment."

    sudo ./setup_hugepage.sh
    sudo ./pktgen.py run
    sudo ./vnf.py run
    if [ "$PM" = true ]; then
        sudo ./power_manager.py run -i $IFACE
        cd $KERN_DIR
        # sudo xdp-loader load -m skb -p /sys/fs/bpf/ enp0s25 ./../kern/xdp_time/xdp_time_kern.o
        sudo ./xdp_time_loader $IFACE
    fi
else
    echo "* Teardown test environment."

    sudo ./pktgen.py stop
    sudo ./vnf.py stop
    sudo ./power_manager.py stop -i $IFACE
    sudo xdp-loader unload $IFACE
fi
