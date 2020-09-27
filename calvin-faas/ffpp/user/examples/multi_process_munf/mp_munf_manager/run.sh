#!/bin/bash

if [[ ! -f "../../../build/examples/ffpp_mp_munf_manager" ]]; then
    echo "ERR: Can not find the built ffpp_mp_munf_manager executable."
    echo "Please run 'make examples' in the ffpp/user folder."
    exit 1
fi

if [[ $1 == "-t" ]]; then
    ../../../build/examples/ffpp_mp_munf_manager -l 1 --proc-type primary --no-pci \
        --single-file-segments --file-prefix=ffpp_mp_munf_manager \
        --log-level=user1,8 \
        --vdev=net_null0 --vdev=net_null1
else
    ../../../build/examples/ffpp_mp_munf_manager -l 1 --proc-type primary --no-pci \
        --single-file-segments --file-prefix=ffpp_mp_munf_manager \
        --vdev net_af_packet0,iface=vnf-in --vdev net_af_packet1,iface=vnf-out
fi
