DPDKConfig {
    -- arbitrary DPDK command line options
    -- the following configuration allows multiple DPDK instances (use together with pciWhitelist)
    -- cf. http://dpdk.org/doc/guides/prog_guide/multi_proc_support.html#running-multiple-independent-dpdk-applications
    cli = {
        "--file-prefix", "moongen",
        "--vdev=virtio_user0,path=/var/run/openvswitch/vhost-user1",
        --"--vdev=eth_af_packet,iface=eth0",
        "--no-pci"
    }

}
