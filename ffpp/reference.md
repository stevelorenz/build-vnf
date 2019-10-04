# Reference Tools, Frameworks or Platforms #

Some tricks and low level optimizations are learned from several high-performance packet I/O and processing frameworks
and integrated into FFPP. These frameworks are awesome, but are relative too heavy, hide many tunable parameters
or have some limitations to simply integrated with data processing frameworks (e.g. Using OpenCV's python bindings to
process images easily). Therefore, I planed to built this light library.

- [VPP](https://wiki.fd.io/view/VPP/What_is_VPP%3F): The VPP platform is an extensible framework that provides out-of-the-box production quality switch/router functionality.

    1. Vectorized processing to avoid L1 cache misses and use a efficient prefetch mechanism.

- [Natasha](https://github.com/scaleway/natasha): Natasha is a fast and scalable, DPDK powered, stateless NAT44 packet processor.

- [ContainerDNS](https://github.com/tiglabs/containerdns): A full cache DNS for Kubernetes.
