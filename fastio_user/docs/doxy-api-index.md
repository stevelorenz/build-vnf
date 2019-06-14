# Fast Packet IO library in User Space (fastio_user) #

This library is developed for **prototyping** high-performance Virtualized Network Functions (VNFs) with data plane
frameworks in the Linux's **user space**. Since this library is used for my PhD research and teaching purposes, it focus
on the **simplicity**.

fastio_user currently focuses on providing DPDK C wrappers that can be used to speed up packet I/O and processing with
more programmer friendly APIs and utility functions.

API {#index}
===

The public API headers are grouped by topics

- *device*:
    [dev]                (@ref device.h),

- *task*:
    [task]               (@ref task.h)

- *containers*:
    [collections]        (@ref collections.h)
