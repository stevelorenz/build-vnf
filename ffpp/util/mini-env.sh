#!/bin/bash
#
# mini-env.sh
# About: Inspired by the testenv.sh in the xdp-tutorial project.
#
#

set -o errexit
set -o nounset
umask 077

NEEDED_TOOLS="ip tc ping"
CLEANUP_FUNC=
CMD=
NS="test"

check_prereq() {
    if [[ "$EUID" -ne "0" ]]; then
        die "Run this script with root permissions."
    fi
}

iface_macaddr() {
    local iface="$1"
    ip -br link show dev "$iface" | awk '{print $3}'
}

die() {
    echo "$1" >&2
    exit 1
}

usage() {
    echo "Usage: $0 [options] <command>"
    echo ""
    echo "Commands:"
    echo "setup                 Setup the test environment."
    echo "teardown              Teardown the test environment."
    exit 1
}

setup() {
    echo "Setting up the environment: $NS."

    local PEERNAME="$NS-in"
    # TODO: Change all address to IPv6
    local INSIDE_IP="192.168.17.2"
    local OUTSIDE_IP="192.168.17.1"
    local OUTSIDE_SUBNET="192.168.17.0"

    if ! mount | grep -q /sys/fs/bpf; then
        mount -t bpf bpf /sys/fs/bpf/
    fi

    ip netns add "$NS"
    ip link add dev "$NS" type veth peer name "$PEERNAME"
    OUTSIDE_MAC=$(iface_macaddr "$NS")
    INSIDE_MAC=$(iface_macaddr "$PEERNAME")

    ethtool -K "$NS" rxvlan off txvlan off
    ethtool -K "$PEERNAME" rxvlan off txvlan off
    ip link set dev "$PEERNAME" netns "$NS"
    ip link set dev "$NS" up
    ip addr add dev "$NS" "${OUTSIDE_IP}/24"

    ip -n "$NS" link set dev "$PEERNAME" name veth0
    ip -n "$NS" link set lo up
    ip -n "$NS" link set veth0 up
    ip -n "$NS" addr add dev veth0 "${INSIDE_IP}/24"

    # Prevent neighbour queries on the link
    ip neigh add "$INSIDE_IP" lladdr "$INSIDE_MAC" dev "$NS" nud permanent
    ip -n "$NS" neigh add "$OUTSIDE_IP" lladdr "$OUTSIDE_MAC" dev veth0 nud permanent
    ip -n "$NS" route add "$OUTSIDE_SUBNET" via "$OUTSIDE_IP" dev veth0

    ip netns exec "$NS" ping -c 1 "$OUTSIDE_IP"
}

teardown() {
    echo "Teardowning the environment: $NS."
    ip link del "$NS"
    ip netns del "$NS"

}

run_tcpdump() {
    ip netns exec "$NS" tcpdump -nei veth0 -v
}

cleanup() {
    [ -n "$CLEANUP_FUNC" ] && $CLEANUP_FUNC
}

OPTS="h"
LONGOPTS="help"

OPTIONS=$(getopt -o "$OPTS" --long "$LONGOPTS" -- "$@")
[ "$?" -ne "0" ] && usage >&2 || true
eval set -- "$OPTIONS"

while true; do
    arg="$1"
    shift
    case "$arg" in
    -h | --help)
        usage >&2
        ;;
    --)
        break
        ;;

    esac
done

[ "$#" -eq 0 ] && usage >&2

case "$1" in
setup | teardown)
    CMD="$1"
    ;;
tcpdump)
    CMD="run_$1"
    ;;
"help")
    usage >&2
    ;;
*)
    usage >&2
    ;;
esac
shift
trap cleanup EXIT
check_prereq
$CMD
