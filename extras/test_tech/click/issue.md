# Issue #

## Compile the kernel module ##

Fail to compile Click Linux module on Vagrant VM with Ubuntu 16.04 (Kernel version 4.4.0-87-generic)

### Error Log

Compile error:
```
/extra_disk/click/linuxmodule/../elements/linuxmodule/fromhost.cc:143:50: error: macro "alloc_netdev" requires 4 arguments, but only 3 given net_device *dev = alloc_netdev(0, name, setup);
```

### Solution

Edit source code: /elements/linuxmodule/fromhost.cc

```
net_device *dev =  alloc_netdev(0, name, NET_NAME_UNKNOWN, setup);
```
