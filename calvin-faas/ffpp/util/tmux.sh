#!/bin/bash
# About: Make the development of two_veth_xdp_fwd setup easier.
#

if [ ! "$EUID" -ne 0 ]; then
    echo "Please run this script without root privilege."
    exit
fi

SESSION_NAME="two_veth_xdp_fwd"
TREX_VER="v2.81"

if [[ $1 == "-c" ]]; then
    sudo ./benchmark-local.py --setup_name two_veth_xdp_fwd --pktgen_image trex:v2.81 teardown
    tmux kill-session -t $SESSION_NAME
    exit 0
fi

set -o errexit
set -o nounset
set -o pipefail

echo "- Create a new Tmux session with two_veth_xdp_fwd setup."
echo "  - The name of the session: two_veth_xdp_fwd"

tmux new-session -d -s $SESSION_NAME -n bash
tmux select-window -t $SESSION_NAME:0
tmux send-keys "sudo ./benchmark-local.py --setup_name two_veth_xdp_fwd --pktgen_image trex:v2.81 setup" C-m
# Wait containers to be created.
sleep 5
tmux new-window -t $SESSION_NAME:1 -n pktgen1
tmux select-window -t $SESSION_NAME:pktgen1
tmux send-keys "sudo docker attach pktgen" C-m
tmux send-keys "cd /trex/$TREX_VER && ./t-rex-64 -i" C-m
tmux new-window -t $SESSION_NAME:2 -n pktgen2
tmux select-window -t $SESSION_NAME:pktgen2
tmux send-keys "sudo docker exec -it pktgen bash" C-m
tmux send-keys "cd /trex/$TREX_VER/local" C-m
tmux new-window -t $SESSION_NAME:3 -n vnf1
tmux select-window -t $SESSION_NAME:vnf1
tmux send-keys "sudo docker attach vnf" C-m
tmux new-window -t $SESSION_NAME:4 -n vnf2
tmux send-keys "sudo docker exec -it vnf bash" C-m

tmux select-window -t $SESSION_NAME:0
tmux attach-session -t $SESSION_NAME
