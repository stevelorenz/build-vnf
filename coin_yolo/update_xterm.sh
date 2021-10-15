#!/bin/env bash

set -e
set -o nounset

if [[ $EUID -eq 0 ]]; then
    echo "Do not run this script with root!"
    exit 1
fi

xrdb -merge ~/.Xresources
