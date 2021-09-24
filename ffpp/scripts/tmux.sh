#!/bin/bash
# About: Make the development of veth_xdp_fwd setup easier.
#

if [ "$EUID" -ne 0 ]; then
    echo "Please run this script with sudo."
    exit
fi

set -o errexit
# set -o nounset
set -o pipefail
umask 077

trap 'pkill -P $$' SIGINT SIGTERM

SESSION_NAME="veth_xdp_fwd"
TREX_VER="v2.81"
NANO_CPUS="5e8" # 50%

usage() {
    echo "Usage: $0 [-n <nano_cpus>] [-c]" 1>&2
    echo "Without any options: run the setup."
    echo "-c: Cleanup the tmux session."
    echo "-n: Set the nano_cpus. E.g. 1e9 for 100%."
    exit 1
}

while getopts "cn:" arg; do
    case "${arg}" in
    c)
        ./benchmark-local.py --setup_name veth_xdp_fwd --pktgen_image trex:$TREX_VER teardown
        tmux kill-session -t $SESSION_NAME
        exit 0
        ;;
    n)
        NANO_CPUS=${OPTARG}
        ;;
    *)
        usage
        ;;
    esac
done
shift $((OPTIND - 1))

echo "- Create a new Tmux session with veth_xdp_fwd setup."
echo "  - The name of the session: veth_xdp_fwd"

tmux new-session -d -s $SESSION_NAME -n bash
tmux select-window -t $SESSION_NAME:0
# The tmux wait-for will block here until the command in this window finishes.
# This is important since following commands depend on this one.
tmux send-keys \
    "./benchmark-local.py --setup_name veth_xdp_fwd \
    --pktgen_image trex:$TREX_VER \
    --nano_cpus $NANO_CPUS \
    setup; tmux wait-for -S setup" C-m\; wait-for setup
tmux new-window -t $SESSION_NAME:1 -n pktgen1
tmux select-window -t $SESSION_NAME:pktgen1
tmux send-keys "docker attach pktgen" C-m
tmux send-keys "cd /trex/$TREX_VER && ./t-rex-64 -i" C-m
tmux new-window -t $SESSION_NAME:2 -n pktgen2
tmux select-window -t $SESSION_NAME:pktgen2
tmux send-keys "docker exec -it pktgen bash" C-m
tmux send-keys "cd /trex/$TREX_VER/local" C-m
tmux new-window -t $SESSION_NAME:3 -n vnf1
tmux select-window -t $SESSION_NAME:vnf1
tmux send-keys "docker attach vnf0" C-m
tmux new-window -t $SESSION_NAME:4 -n vnf2
tmux send-keys "docker exec -it vnf0 bash" C-m

tmux select-window -t $SESSION_NAME:0
tmux attach-session -t $SESSION_NAME
