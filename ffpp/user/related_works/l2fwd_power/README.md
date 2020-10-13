# L2 Forwarding Sample Application with Power Management

A modified l2fwd sample application with the power management (PM) mechanisms ported from l3fwd-power sample
application.

Differences from the PM used in l3fwd-power:

-   The mapping between lcores, ports and queues are hard-coded instead of being configurable with --config argument.
    So except for the main core, each worker cores handles the ONLY first queue of its port.

-   Currently, only `APP_MODE_LEGACY` is implemented. veth interface does not support interrupt registration and RX
    descriptor counting.


```bash
cd ../../
make related_works
cd ./related_works/l2fwd_power
./run.sh -t
```
