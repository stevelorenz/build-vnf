Development Environment
=======================

.. role:: bash(code)
    :language: bash

Assume the root directory of the project is :file:`$BUILD_VNF_ROOT`

Vagrant_ is used to make development environments (Dev-Env) easy and safe (You
know what I mean after using :bash:`vagrant destroy` and :bash:`vagrant up`).
Currently, all environments are built as Virtual Machines (VMs) running on the
Virtualbox_ hypervisor.  If you are a Linux user, install vagrant and
virtualbox with your package manager. Then you can setup the required
development with configurations written in Vagrantfile.

Here the basics boxes like `bento/ubuntu-16.04` are used to spawn the VM and Bash scripts (Basic and shared ones are
included in the Vagrantfile, more complex ones are located in the :file:`$BUILD_VNF_ROOT/scripts`) are used to setup the
development environment. Check the Vagrantfile located in the project root directory for configuration details.


Setup OpenCV Dev-Env:
"""""""""""""""""""""
.. code:: bash

    cd ~/build-vnf
    vagrant up sim
    vagrant ssh sim
    bash /vagrant/script/install_opencv.sh

The OpenCV source code is compiled in the last step which takes time. Only python3 bindings are installed.
After the successful compiling and installation, run the edge sample application for testing.

.. code:: bash

    cd ~/opencv/samples/python
    python3 ./edge.py


DPDK accelerated Docker
"""""""""""""""""""""""

Run DPDK Hello World example in Docker container
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Docker container configuration files (Dockerfile) and scripts are located in :file:`$BUILD_VNF_ROOT/testbed/containertb`.
The Vagrant VM **containertb** is designed to build the development environment. Run fowllowing commands to create a new
VM for container testsbed.

.. code:: bash

  vagrant up containertb
  vagrant ssh containertb
  cd /vagrant/script/
  bash ./install_docker.sh
  bash ./install_ovs_dpdk.sh

Now the setup of container testbed is finished. In order to use OVS-DPDK and also DPDK inside the container, some
configurations including allocating hugepages, load kernel modules and bind interface to the DPDK need to be executed
every time the VM is booted.

.. code:: bash

  vagrant up containertb
  vagrant ssh containertb
  cd /vagrant/script/
  bash ./setup_ovs_dpdk.sh

The docker image need to be built for the first time.

.. code:: bash

  cd /vagrant/testbed/containertb/
  # This step takes time.
  sudo docker build -t dpdk_test:v0.1 -f ./Dockerfile.dpdk_minimal .
  sudo docker image ls

When the image is successfully built, you can test the setup by running the DPDK helloworld example application with the
test script.

.. code:: bash

    cd /vagant/testbed/containertb/
    bash ./run_dpdk_in_docker.sh

    # Output example
    EAL: Multi-process socket /var/run/.rte_unix
    EAL: Probing VFIO support...
    EAL: Detected 2 lcore(s)
    EAL: PCI device 0000:00:03.0 on NUMA socket -1
    EAL:   Invalid NUMA socket, default to 0
    EAL:   probe driver: 8086:100e net_e1000_em
    EAL: PCI device 0000:00:08.0 on NUMA socket -1
    EAL:   Invalid NUMA socket, default to 0
    EAL:   probe driver: 8086:100e net_e1000_em
    hello from core 1
    hello from core 0

Congrates! Now you can develop your DPDK application and package it with docker. For chaining multiple Docket containers
to build a Service Function Chain. Check the next section.

.. _Vagrant: https://www.vagrantup.com/
.. _Virtualbox: https://www.virtualbox.org/wiki/Downloads

