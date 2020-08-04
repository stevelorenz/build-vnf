#!/bin/bash

# Mainline Linux kernel has mlx5_core driver.
# echo "- Download MLNX_OFED driver."

function usage() {
    echo "Usage: ./setup.sh <Option>"
    echo "  -u: Update the firmware."
    exit 1
}

function update_firmware() {
    echo "- Check and update firmware (get FW images online from Mellanox server)."
    if [[ ! -f ./mlxup ]]; then
        wget -O mlxup http://www.mellanox.com/downloads/firmware/mlxup/4.14.4/SFX/linux_x64/mlxup
        chmod u+x ./mlxup
    fi
    ./mlxup --update --online
}

while getopts ":hu" arg; do
    case "${arg}" in
    "u")
        update_firmware
        exit 0
        ;;
    h | *)
        usage
        exit 0
        ;;
    esac
done

shift $((OPTIND - 1))
