# DPDK #

## Catalog ##

- workflow.md: Instructions for installing and configuring DPDK in the test environment.
- dev_guide.md: Instructions for developing applications on DPDK.
- utils: Useful tool scripts.
- examples: Official DPDK example applications. Used for learning and testing.
- app: Self-developed DPDK application.

    - logaddr: Print MAC and IP addresses of all received packets.
    - udp_append_ts: Append processing timestamp to all received UDP packets.

- pktgen: Scripts and configs for the traffic generator --- pktgen.
- monitor: Tools for performance monitoring.

## Build and run app ##

All apps use the same structure of official examples. Building rules are written in makefiles.

1. To enable debugging with gdb, export the global `EXTRA_CFLAGS`:
```
export EXTRA_CFLAGS="-O0 -g"
```

2. To enable packet capturing with pdump, the DPDK should compiled with lipcap support and the lipcap-dev should be
installed. Check the info [here](https://dpdk.org/doc/guides/howto/packet_capture_framework.html).

3. Running the udp_append_ts app with packet capturing of all tx packets.

```
# Shell 1
$ cd ./app/udp_append_ts/
$ make
$ sudo ./build/udp_append_ts -l 0 -- -p 0x3 -q 2 -d 9c:da:3e:6f:ba:df --packet-capturing

# Shell 2
sudo "${RTE_SDK}/build/app/dpdk-pdump" -- \
--pdump 'port=1,queue=*,tx-dev=./p1_tx.pcap'
```

> Other generic options for DPDK application can be found [here](http://dpdk.org/doc/guides/linux_gsg/build_sample_apps.html#running-a-sample-application).
