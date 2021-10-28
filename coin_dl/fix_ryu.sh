#!/bin/bash

if [ "$EUID" -ne 0 ]; then
    echo "Please run with sudo."
    exit
fi

# ISSUE: Ryu manager does not work with the latest eventlet...
# Have to downgrade the package for a "quickfix"...
pip3 install eventlet==0.30.2
