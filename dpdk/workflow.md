# Workflow for Setup DPDK Development Environment #

## 1. Setup a Vagrant VM ##

The configurations of VM is written in ../Vagrantfile. Run `vagrant up` in the same path of the Vagrantfile to setup
the VM. Additional interfaces in host-only mode are added to be bound to the DPDK. By default, the `eth0` is generated
by VBox with NAT mode and also used by vagrant for ssh. So other interfaces can be bound to DPDK for testing.

When the VM is fully up, run `vagrant ssh` command to connect to the VM, the default user is vagrant and can use sudo
without password.

- the root directory (where Vagrantfile is located ) is mapped to the `\vagrant` on the VM

## 2. Run ./setup.sh ##

```
$ bash /vagrant/dpdk/setup.sh
```

This script installs dependency packages, configure hugepages, download DPDK source code, compile DPDK and install
proper kernel modules.  At the end, required ENV variables RTE_SDK and RTE_Target is also added into ./profile to keep
them persistent after rebooting.

## 3. Run ./bind.sh ##

```
$ bash /vagrant/dpdk/bind.sh
```

This script allocates hugepages and bind `eth1` to DPDK for testing. SHOULD be executed after each reboot.

## 4. Compile and run helloworld example ##

```
$ cd /vagrant/dpdk/helloworld
$ make
$ cd /vagrant/dpdk/helloworld/build
$ sudo ./helloworld -l 0
```

* -l CORELIST: is a set of core numbers on which the application runs.


Other generic opetions for DPDK application can be found [here](http://dpdk.org/doc/guides/linux_gsg/build_sample_apps.html#running-a-sample-application).
