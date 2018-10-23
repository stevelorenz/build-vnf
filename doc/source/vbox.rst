Vagrant Boxes
=============

.. role:: bash(code)
    :language: bash

Assume the root directory of the project is `~/build-vnf`

Vagrant_ is used to make development environments (Dev-Env) easy and safe (You
know what I mean after using :bash:`vagrant destroy` and :bash:`vagrant up`).
Currently, all environments are built as Virtual Machines (VMs) running on the
Virtualbox_ hypervisor.  If you are a Linux user, install vagrant and
virtualbox with your package manager. Then you can setup the required
development with configurations written in Vagrantfile.

Here the basics boxes like `bento/ubuntu-16.04` are used to spawn the VM and
Bash scripts (Basic and shared ones are included in the Vagrantfile, more
complex ones are located in the `~/build-vnf/scripts`) are used to setup the
development environment. Check the Vagrantfile located in the project root
directory for configuration details.


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

.. _Vagrant: https://www.vagrantup.com/
.. _Virtualbox: https://www.virtualbox.org/wiki/Downloads
