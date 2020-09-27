#!/bin/bash

if [[ ! -f "../../../build/examples/ffpp_mp_munf_munf" ]]; then
    echo "ERR: Can not find the built ffpp_mp_munf_munf executable."
    echo "Please run 'make examples' in the ffpp/user folder."
    exit 1
fi

# INFO: Secondary process MUST run on a different core as the primary process.
../../../build/examples/ffpp_mp_munf_munf -l 2 --proc-type secondary --no-pci \
    --single-file-segments --file-prefix=ffpp_mp_munf_manager \
    -- \
    -f 0
