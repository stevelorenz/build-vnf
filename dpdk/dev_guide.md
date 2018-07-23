# Workflow for Setup DPDK Development Environment #

## 1. Setup Vagrant VMs ##

The configurations of VMs are written in ../Vagrantfile. Run `vagrant up` in the same path of the Vagrantfile to setup
the VM. Additional interfaces in host-only mode are added to be bound to the DPDK. By default, the `eth0` is generated
by VBox with NAT mode and also used by vagrant for ssh. So other interfaces can be bound to DPDK for testing.

When the VMs is fully up, run `vagrant ssh vnf` command to connect to the VM running the VSF program, the default user
is vagrant and can use sudo without password. Run `vagrant ssh trafficgen` command to connect to the VM running the
traffic generator.

- the root directory (where Vagrantfile is located ) is mapped to the `\vagrant` on each VM.

## 2. Run ./setup.sh ##

```
$ bash /vagrant/dpdk/setup.sh
```

This script installs dependency packages, configure hugepages, download DPDK source code, compile DPDK and install
proper kernel modules.  At the end, required ENV variables RTE_SDK and RTE_Target is also added into ./profile to keep
them persistent after rebooting.

MARK: Additional EXTRA_CFLAGS is set to "-O0 -g" to enable debugging DPDK application with gdb.

## 3. Run ./bind.sh ##

```
$ bash /vagrant/dpdk/utils/bind.sh
```

This script allocates hugepages and bind `eth1` and `eth2` to DPDK for testing. SHOULD be executed after each reboot.

## 4. Compile and run helloworld example ##

```
$ cd /vagrant/dpdk/examples/helloworld
$ make
$ cd /vagrant/dpdk/examples/helloworld/build
$ sudo ./helloworld -l 0,1
```

* -l CORELIST: is a set of core numbers on which the application runs.


Other generic options for DPDK application can be found [here](http://dpdk.org/doc/guides/linux_gsg/build_sample_apps.html#running-a-sample-application).
