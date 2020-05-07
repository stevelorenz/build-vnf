# Build Virtualized Network Functions (VNFs) (This is a research/exploration project)

## A collection of utilities to build, test and benchmark practical and high-performance VNFs on a single laptop.

**Warning**: This project is still **under heavy development**.
The directories are not cleanly and clearly organized.
Redundant directories will be removed before the stable 1.0 version.

This project is hosted on [GitHub](https://github.com/stevelorenz/build-vnf) and [BitBucket](https://bitbucket.org/comnets/build-vnf/src/master/). 
Please create an issue or a pull request if you have any questions or code contribution.

## Introduction

This repository is a **mono-repo collection** of programs/libraries/scripts/utilities related to my PhD topic:
Enabling Ultra Low Latency Network Softwarization with COmputing In the Network (COIN).


These programs are used to prototype and evaluate innovative ideas and practical designs for ultra low latency data and
management plane (**focus on data plane**) of Virtualized Network Function (VNFs) running inside containers (CNCF names
this type of NFs to Cloud-native Network Functions (CNFs)) or inside VMs. These VNFs are deployed mainly on Commercial
off-the-shelf (COTS) servers.

The ultra low latency is a fundamental requirement for the [Tactile Internet](https://www.telekom.com/en/company/details/tactile-internet-563646).
According to the 5G communication standard for automation in vertical domains, the allowed end-to-end communication
latency should be lower than **1ms**.
Based on the latency budget described in [this paper](https://ieeexplore.ieee.org/abstract/document/8672612), the
virtualized edge computing system has only **0.35ms** for communication and computation.
Furthermore, one killer application for Tactile Internet is to enable Human-machine Co-Working.
In this use-case, the service delay of **each discrete packet (or message)** play an important role.
According to the [networking requirements of Franka Emika's Robot Arm](https://frankaemika.github.io/docs/requirements.html#network),
the latency of the full control loop should less than 1ms.

> If the <1 ms constraint is violated for a cycle, the received packet is dropped by FCI. After 20 consecutively dropped packets, your robot will stop.

Therefore, the latency of each discrete packet should be measured as the KPI for this project.
Besides latency, other important performance metrics including throughput, power efficiency should also be considered in
the VNF design and implementation.

This is a PhD student's work, so the code is definitely not production-ready.
However, I will try my best to adapt my work to the latest high performance softwarization data plane and management
technologies including DPDK, eBPF/XDP; Containerization, Serverless computation etc.

## Catalog

1.  [CALVIN](./CALVIN/): Programs and scripts used in [JSAC CALVIN paper](https://ieeexplore.ieee.org/abstract/document/8672612).
1.  [CALVIN-FaaS](./calvin-faas/): A system (**under-development**) for the idea of serverless NFV.
1.  [perf_eval](./perf_eval/): Tools to monitor and profiling the underlying physical or virtual systems running VNFs.
1.  [pktgen](./pktgen/):  Utilities to configure and run software packet generators for VNF performance benchmarking.
1.  [scripts](./scripts/): Utility shell/python scripts to install, setup, configure tools used in this project.


### To be cleaned/removed Directories

1.  [shared_lib](./shared_lib/): Initially used to put shared libraries used by different VNFs. Should be integrated into ffpp library.

## Vagrant-managed Virtual Machine for Test and Development

Most programs in this project are developed and tested inside a VM managed by Vagrant.

#### Use Virtualbox as the Provider

The development, test and basic benchmarks of VNFs, libraries are performed on a pre-configured VM managed by the
Vagrant. The receipt to build the VM is in the [Vagrantfile](./Vagrantfile).

Recommended setup:

- Vagrant: v2.2.5 and beyond ([Download Link](https://www.vagrantup.com/downloads.html))
- Virtualbox: v6.0 and beyond ([Download Link](https://www.virtualbox.org/wiki/Downloads))

Once the Vagrant and Virtualbox are installed, you can open a terminal (or CMD/powershell on Windows) on your host OS
and change the current working directory to the build-vnf source directory which contains the Vagrantfile.
Following commands can be used to manage the VM:

```bash
# Print current machine states
$ vagrant status

# Create the VM for VNF development
$ vagrant up vnf

# Log into the VM via SSH
$ vagrant ssh vnf
```
All tests and benchmarks should run **inside** the VM.
And the path of files in READMEs are also paths **inside** the VM.

#### Use KVM as the Provider

On a host OS with Linux kernel (e.g. GNU/Linux distributions), KVM can be used as the VM hypervisor.
Compared to the Virtualbox, KVM (as the native para-virtualization technology provided by the Linux kernel), generally
provides better performance, time accuracy and hardware-access.
For example, the VM could access the physical CPU directly to control its frequency states.

Vagrant uses Libvirt to manage KVM powered VMs. The Libvirt plugin of Vagrant with its dependencies (e.g. qemu, kvm
etc.) must be installed correctly.
To create the VM with Libvirt provider.
Check the [guide](https://github.com/vagrant-libvirt/vagrant-libvirt#installation) to install the plugin and its
dependencies.
After successful installation, run the following command in the build-vnf's source directory:

```bash
$ vagrant up --provider libvirt vnf
# Remove the VM managed by libvirt, the default provider is Virtualbox. Use VAGRANT_DEFAULT_PROVIDER to force vagrant to
# use libvirt.
$ VAGRANT_DEFAULT_PROVIDER=libvirt vagrant destroy vnf
```

## Contribution ##

This project exists thanks to all people who contribute.
The [list](./CONTRIBUTORS) of all contributors.

## Contact ##

* **Zuo Xiang** - *Project Maintainer* - xianglinks@gmail.com
