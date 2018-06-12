# Issue #

## Compile the kernel module ##

### Problem

On Ubuntu 16.04 with kernel 4.4.0-116-generic, I tried to compile the click kernel module with the following command:
./configure --enable-linuxmodule --with-linux=/usr/src/linux-headers-4.4.0-116-generic --with-linux-map=/boot && sudo make install

I got the following compile error:
/extra_disk/click/linuxmodule/../elements/linuxmodule/fromhost.cc:143:50: error: macro "alloc_netdev" requires 4 arguments, but only 3 given net_device *dev = alloc_netdev(0, name, setup);

### Solution

Edit source code: /elements/linuxmodule/fromhost.cc

```
net_device *dev =  alloc_netdev(0, name, NET_NAME_UNKNOWN, setup);
```
