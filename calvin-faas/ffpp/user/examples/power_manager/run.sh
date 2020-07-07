#!/bin/bash
# About: Run power manager application in the benchmark setup described in the ffpp/util.
#

if [[ ! -f "../../build/examples/ffpp_power_manager" ]]; then
    echo "ERR: Can not find the built ffpp_power_manager executable."
    echo "Please run 'make examples' in the ffpp/user folder."
    exit 1
fi

EAL_OPTS="-l 2 --no-pci --single-file-segments --file-prefix=power_manager"
printf "* Run power manager example:\n-The EAL options: %s\n" "$EAL_OPTS"

../../build/examples/ffpp_power_manager $EAL_OPTS
