DPDKConfig {
    -- arbitrary DPDK command line options
    -- the following configuration allows multiple DPDK instances (use together with pciWhitelist)
    -- cf. http://dpdk.org/doc/guides/prog_guide/multi_proc_support.html#running-multiple-independent-dpdk-applications
    cli = {
        "--file-prefix", "moongen",
        "--vdev=eth_af_packet,iface=vnf-in-out",
        "--no-pci"
    }

}
