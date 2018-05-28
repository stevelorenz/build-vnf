# -*- mode: ruby -*-
# vi: set ft=ruby :
# About: Vagrant file for the development environment
#        The measurements should be performed on the OpenStack cloud environment.
#
# Component: - Traffic Generator: trafficgen
#            - VNF: vnf

###############
#  Variables  #
###############

CPUS = 2
RAM = 2048

BOX = "bento/ubuntu-16.04"

####################
#  Vagrant Config  #
####################

Vagrant.configure("2") do |config|

    # --- VM for VNF ---
    config.vm.define "vnf" do |vnf|
        vnf.vm.box = BOX
        vnf.vm.hostname = "vnf"

        # Create a private network, which allows host-only access to the machine using a specific IP.
        # This option is needed otherwise the Intel DPDK takes over the entire adapter
        vnf.vm.network "private_network", ip: "10.0.0.11"
        vnf.vm.network "private_network", ip: "10.0.0.12"
        vnf.vm.provision :shell, path: "bootstrap.sh"

        # VirtualBox-specific configuration
        vnf.vm.provider "virtualbox" do |vb|
            vb.name = "ubuntu-16.04-vnf"
            vb.memory = RAM
            vb.cpus = CPUS
            # MARK: The CPU should enable SSE3 or SSE4 to compile DPDK
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
        end
    end

    # --- VM for Traffic Generator ---
    config.vm.define "trafficgen" do |trafficgen|
        trafficgen.vm.box = BOX
        trafficgen.vm.hostname = "trafficgen"

        # Create a private network, which allows host-only access to the machine using a specific IP.
        # This option is needed otherwise the Intel DPDK takes over the entire adapter
        trafficgen.vm.network "private_network", ip: "10.0.0.13"
        trafficgen.vm.network "private_network", ip: "10.0.0.14"
        trafficgen.vm.provision :shell, path: "bootstrap.sh"

        # VirtualBox-specific configuration
        trafficgen.vm.provider "virtualbox" do |vb|
            # Set easy to remember VM name
            vb.name = "ubuntu-16.04-trafficgen"
            vb.memory = RAM
            vb.cpus = CPUS
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
        end
    end

end
