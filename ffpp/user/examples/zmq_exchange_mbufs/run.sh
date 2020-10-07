#!/bin/bash

if [[ ! -f "../../build/examples/ffpp_zmq_exchange_mbufs" ]]; then
    echo "ERR: Can not find the built ffpp_zmq_exchange_mbufs executable."
    echo "Please run 'make examples' in the ffpp/user folder."
    exit 1
fi

if [[ $1 == "-t" ]]; then
    ../../build/examples/ffpp_zmq_exchange_mbufs -l 1 --proc-type primary --no-pci \
        --single-file-segments --file-prefix=ffpp_zmq_exchange_mbufs \
        --vdev=net_null0 --vdev=net_null1
else
    ../../build/examples/ffpp_zmq_exchange_mbufs -l 1 --proc-type primary --no-pci \
        --single-file-segments --file-prefix=mp_munf_mono \
        --vdev net_af_packet0,iface=vnf-in --vdev net_af_packet1,iface=vnf-out
fi
