#! /bin/bash
# About: Run test click vnfs

function print_help() {
    echo "Usage:"
    echo "$ ./run_click.sh option"
    echo "  -uf: Run UDP forwarding in user level."
    echo "  -ua: Run UDP appending timestamps in user level."
    echo "  -ux: Run UDP XORing in user level."
}

if [[ $1 = '-uf' ]]; then
    # Run Click in user-space
    sudo click -t -h udp_ctr.count ./udp_forwarding.click
elif [[ $1 = '-ua' ]]; then
    sudo click -t -h udp_ctr.count ./udp_append_ts.click
elif [[ $1 = '-ux' ]]; then
    sudo click -t -h udp_ctr.count ./udp_xor.click
elif [[ $1 = '-h' ]]; then
    print_help
else
    echo "Invalid option!"
    print_help
fi
