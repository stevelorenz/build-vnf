# FFPP - Fast fast Packet Processing Library #

This library is developed for **fast prototyping** and **teaching** high performance packet processing inside Linux containers.
To be utilized low-level technologies including:

-   [DPDK](https://www.dpdk.org/): Fast packet processing in user space.
-   [XDP](https://www.iovisor.org/technology/xdp): Fast network data path in the Linux kernel.

Check the [README page](@ref README.md) to get started.

API {#index}
============

The public API headers are currently grouped by topics:

- *Task*:
    [task]               (@ref task.h)

- *Device*:
    [dev]                (@ref device.h),

- *Packet IO*:
    [io]                 (@ref io.h),

- *Data structures and containers*:
    [collections]        (@ref collections.h)

- *Inter-process communication*:
    [ipc]                (@ref ipc.h)

- *Utility functions*:
    [utils]              (@ref utils.h)
