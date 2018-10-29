#! /bin/bash
#
# run_pktgen.sh
#
# About: Run pktgen app inside container
#

echo "# Run testpmd application."
cd "$RTE_SDK/build/app" || exit

EAL_PARAMS="-l 0,1 -n 1 -m 512 \
    --file-prefix pktgen \
    --no-pci \
    --single-file-segments \
    --vdev=virtio_user2,path=/var/run/openvswitch/vhost-user0 \
    --vdev=virtio_user3,path=/var/run/openvswitch/vhost-user1
"
PKTGEN_PARAMS="-T -P -m "1.0,1.1""

/root/pktgen-dpdk/app/"$RTE_TARGET"/pktgen $EAL_PARAMS -- $PKTGEN_PARAMS
