#!/bin/bash

if [[ $# -ne 1 ]]; then
    echo 'Require argument. 0 to turn off turbo_boost, 1 to turn it on'
    exit 1
fi

for i in {0..7}; do
    state=$(sudo rdmsr -p${i} 0x1a0 -f 38:38 2>/dev/null)
    if [[ $state ]]; then
        if [[ $1 == 0 ]]; then
            sudo wrmsr -p${i} 0x1a0 0x4000850089
        elif [[ $1 == 1 ]]; then
            sudo wrmsr -p${i} 0x1a0 0x850089
        else
            echo 'Invalid argument. 0 to turn off turbo_boost, 1 to turn it on'
            exit 1
        fi
        state=$(sudo rdmsr -p${i} 0x1a0 -f 38:38) &>/dev/null
        if [[ $state -eq 1 ]]; then
            echo "Core ${i} disabled"
        else
            echo "Core ${i} enabled"
        fi
    else
        echo "Core ${i} not available"
    fi
done
