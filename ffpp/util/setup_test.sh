#! /usr/bin/env bash
#
# About: Setup pktgen, vnf and power-manager and loads the XDP hook

IFACE="enp5s0f0"
CORE="5" # For XDP program
SETUP=true
PM=true
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
    -c)
        if [ "$2" ]; then
            CORE="$2"
            shift
        fi
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
    # sudo ./pktgen.py run
    if [ "$PM" = true ]; then
        sudo ./vnf.py run
        sudo ./power_manager.py run -i $IFACE -c $CORE
        cd $KERN_DIR
        # sudo xdp-loader load -m skb -p /sys/fs/bpf/ enp0s25 ./../kern/xdp_time/xdp_time_kern.o
        taskset --cpu-list $CORE sudo ./xdp_time_loader $IFACE
    else
        sudo ./vnf.py run --no_pm
    fi
else
    echo "* Teardown test environment."

    # sudo ./pktgen.py stop
    sudo ./vnf.py stop
    sudo ./power_manager.py stop -i $IFACE
    # sudo xdp-loader unload $IFACE
fi
