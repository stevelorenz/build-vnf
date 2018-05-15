# -*- mode: ruby -*-
# vi: set ft=ruby :
# About: Vagrant file for the test VM

Vagrant.configure("2") do |config|

    config.vm.box = "bento/ubuntu-16.04"
    config.vm.hostname = "vnf"

    # Create a private network, which allows host-only access to the machine using a specific IP.
    # This option is needed otherwise the Intel DPDK takes over the entire adapter
    config.vm.network "private_network", ip: "10.0.0.11"
    config.vm.network "private_network", ip: "10.0.0.12"
    config.vm.provision :shell, path: "bootstrap.sh"

    # VirtualBox-specific configuration
    config.vm.provider "virtualbox" do |vb|
        # Set easy to remember VM name
        vb.name = "ubuntu-16.04-vnf"
        # Assign 2 GB of memory
        vb.memory = 2048
        # Assign 2 cores
        vb.cpus = 2
        # Configure VirtualBox to enable
        vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
        vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
    end
end
