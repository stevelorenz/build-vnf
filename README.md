[![Build Status](https://travis-ci.com/stevelorenz/build-vnf.svg?branch=master)](https://travis-ci.com/stevelorenz/build-vnf)
![MIT licensed](https://img.shields.io/github/license/stevelorenz/build-vnf)


<p align="center">
<img alt="Build-VNF" src="https://github.com/stevelorenz/build-vnf/raw/master/logo/logo_horizontal.png" width="500">
</p>

A collection (mono-repo) of utilities to build, test and benchmark practical and high-performance and portable Virtualized Network Functions (VNFs) on a single laptop.
This is a exploration project for research purposes.

**Warning**: This project is currently **under heavy development**.
The project structure is not yet stabilized.
Directories will be better organized after the stable 1.0 version.

This project is hosted both on [GitHub](https://github.com/stevelorenz/build-vnf) and [BitBucket](https://bitbucket.org/comnets/build-vnf/src/master/). 
Please create an issue or a pull request if you have any questions or code contribution.


## Table of Contents

*   [Overview](#overview)
*   [Catalog](#catalog)
*   [Quick Start](#quick-start)
*   [Citing Our Works](#citing-our-works)
*   [Contributing](#contributing)
*   [Contact](#contact)
*   [License](#license)


## Overview

This repository is a **collection(mono-repo)** of programs/libraries/scripts/utilities related to my PhD topic: Ultra **Low Latency** Network Softwarization for COmputing In the Network (COIN).
It provides utilities to build, test and benchmark practical and high-performance and portable Virtualized Network Functions (VNFs) or Cloud-native Network Functions
(CNFs) on a single laptop.
The focus of this project is to explore **low-latency**, **energy-efficient** and flexible network **data plane** solutions for softwarized edge computing systems.
This project focuses on (still **WIP**):

-  Lightweight cloud-native network functions running on CPUs or [SmartNICs](https://www.mellanox.com/products/smartnic): The design is based on microservice or
   serverless architecture. Accelerated data plane and network stack works for virtual interfaces with eBPF/XDP support (e.g. veth-pairs, virtio\_net).

-  The bottom-up system design: To minimize the latency of each level.

-  Low per-packet latency, energy efficiency and flexibility: Intelligent network functions should processing packets fast without wasting unnecessary energy.

-  Primitives, APIs and protocols support for COmputing In the Network (COIN) applications.

-  Rigorous and practical performance benchmarking and analyze using both emulator and hardware testbed.

## Catalog

-   [CALVIN](./CALVIN/): Programs and scripts used in [JSAC CALVIN paper](https://ieeexplore.ieee.org/abstract/document/8672612).
-   [FFPP](./ffpp/): A library for fast packet processing with both DPDK and XDP.
-   [pktgen](./pktgen/):  Utilities to configure and run software packet generators for VNF performance measurements.
-   [scripts](./scripts/): Utility shell/python scripts to install, setup, configure tools used in this project.


## Quick Start

The easiest and most convenient way to try `Build-VNF` utilities is to use the test VM managed by `Vagrant`.

The development, test and basic benchmarks of VNFs, libraries are performed on a pre-configured VM managed by the Vagrant.
The receipt to build the VM is in the [Vagrantfile](./Vagrantfile).
`Virtualbox` is used as the default VM provider.

Required setup on the host system:

-   [Vagrant](https://www.vagrantup.com/downloads.html): v2.x.y (tested under v2.2.9)
-   [Virtualbox](https://www.virtualbox.org/wiki/Downloads): v6.x (tested under v6.1.12)
-   Resources for the VM provision: Two CPUs and 2GB RAM.

Once the Vagrant and Virtualbox are installed, you can open a terminal (or CMD/powershell on Windows) on your host OS and change the current working directory to the
`build-vnf` **source directory** which contains the Vagrantfile.
Following commands can be used to manage the VM:

```bash
# Print current machine states
$ vagrant status

# Create the VM for VNF development
$ vagrant up vnf

# Log into the VM via SSH
$ vagrant ssh vnf
```
By default, the `build-vnf` source directory is synced with `/vagrant/` folder inside the VM.
Once inside the created VM, `FFPP` library can be built and tested with steps described [here](./ffpp/README.md).

#### Use KVM as the Provider

On a host OS with Linux kernel (e.g. GNU/Linux distributions), KVM can be used as the VM hypervisor.
Compared to the Virtualbox, KVM (as the native para-virtualization technology provided by the Linux kernel), generally provides better performance, time accuracy and
hardware-access.
For example, the VM could access the physical CPU directly to control its frequency states.

Vagrant uses Libvirt to manage KVM powered VMs. The Libvirt plugin of Vagrant with its dependencies (e.g. qemu, kvm etc.) must be installed correctly.
To create the VM with Libvirt provider, check the [guide](https://github.com/vagrant-libvirt/vagrant-libvirt#installation) to install the plugin and its dependencies.
After successful installation, run the following command in the build-vnf's source directory:

```bash
$ vagrant up --provider libvirt vnf
# Remove the VM managed by libvirt, the default provider is Virtualbox. Use VAGRANT_DEFAULT_PROVIDER to force vagrant to
# use libvirt.
$ VAGRANT_DEFAULT_PROVIDER=libvirt vagrant destroy vnf
```

[Rsync mode](https://www.vagrantup.com/docs/synced-folders/rsync.html) is used in the Libvirt provider for unidirectional (host->guest) sync. 
The syncing does **NOT** automatically start after the `vagrant up`.
Open a terminal, enter the root directory of `build-vnf` project and run `$ vagrant rsync-auto vnf` after the VM  is booted to start syncing

For bidirectional sync, [NFS](https://www.vagrantup.com/docs/synced-folders/nfs.html) need to be configured and used as the sync type.
NFS-based sync tends to have more bugs, so rsync mode is used by default.

## Citing Our Works

If you like tools inside this repository, please cite our papers.

```
@article{xiang2019reducing,
  title={Reducing latency in virtual machines: Enabling tactile internet for human-machine co-working},
  author={Xiang, Zuo and Gabriel, Frank and Urbano, Elena and Nguyen, Giang T and Reisslein, Martin and Fitzek, Frank HP},
  journal={IEEE Journal on Selected Areas in Communications},
  volume={37},
  number={5},
  pages={1098--1116},
  year={2019},
  publisher={IEEE}
}
```


## Contributing

This project exists thanks to all people who contribute.
The [list](./CONTRIBUTORS) of all contributors.

## Contact

* **Zuo Xiang** - *Project Maintainer* - xianglinks@gmail.com

## License

This project is licensed under the [MIT license](./LICENSE).
