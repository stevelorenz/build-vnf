# -*- mode: ruby -*-
# vi: set ft=ruby :
# About: Vagrant file for the development environment
#        The measurements should be performed on the OpenStack cloud environment.

###############
#  Variables  #
###############

CPUS = 2
RAM = 2048

BOX = "bento/ubuntu-16.04"
BOX_1404 = "bento/ubuntu-14.04"
BOX_1404_316 = "novael_de/ubuntu-trusty64"

####################
#  Vagrant Config  #
####################

Vagrant.configure("2") do |config|

    # --- VM for DPDK development ---
    config.vm.define "dpdk" do |dpdk|
        dpdk.vm.box = BOX
        dpdk.vm.hostname = "dpdk"
        # Create private networks, which allows host-only access to the machine using a specific IP.
        # This option is needed otherwise the Intel DPDK takes over the entire adapter
        # 10.0.0.11 is always used for receiving packets ---> Use fix MAC address
        dpdk.vm.network "private_network", ip: "10.0.0.11", mac: "080027baf4c6"
        dpdk.vm.network "private_network", ip: "10.0.0.12", mac: "0800273c9768"
        dpdk.vm.provision :shell, path: "bootstrap.sh"

        # VirtualBox-specific configuration
        dpdk.vm.provider "virtualbox" do |vb|
            vb.name = "ubuntu-16.04-dpdk"
            vb.memory = RAM
            vb.cpus = CPUS
            # MARK: The CPU should enable SSE3 or SSE4 to compile DPDK
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
        end
    end

    # --- VM for BCC development ---
    config.vm.define "bcc" do |bcc|
        bcc.vm.box = BOX
        bcc.vm.hostname = "bcc"

        # MARK: Virtio supports XDP with Kernel version after 4.10
        bcc.vm.network "private_network", ip: "10.0.0.15",
            nic_type: "82540EM"
        bcc.vm.network "private_network", ip: "10.0.0.16",
            nic_type: "82540EM"
        bcc.vm.provision :shell, path: "bootstrap.sh"

        # VirtualBox-specific configuration
        bcc.vm.provider "virtualbox" do |vb|
            vb.name = "ubuntu-16.04-bcc"
            vb.memory = RAM
            vb.cpus = CPUS
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
            vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
        end
    end

    # --- VM for Click user space driver development ---
    config.vm.define "click" do |click|
        click.vm.box = BOX
        click.vm.hostname = "click"

        click.vm.network "private_network", ip: "10.0.0.17",
            nic_type: "82540EM"
        click.vm.network "private_network", ip: "10.0.0.18",
            nic_type: "82540EM"
        click.vm.provision :shell, path: "bootstrap.sh"

        # VirtualBox-specific configuration
        click.vm.provider "virtualbox" do |vb|
            vb.name = "ubuntu-16.04-click"
            vb.memory = RAM
            vb.cpus = CPUS
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
            vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
        end
    end

    # --- VM for Click Kernel driver development ---
    config.vm.define "click_kernel" do |click_kernel|
        click_kernel.vm.box = BOX_1404_316
        click_kernel.vm.hostname = "clickkernel"

        click_kernel.vm.network "private_network", ip: "10.0.0.19",
            nic_type: "82540EM"
        click_kernel.vm.network "private_network", ip: "10.0.0.20",
            nic_type: "82540EM"
        click_kernel.vm.provision :shell, path: "bootstrap.sh"

        # VirtualBox-specific configuration
        click_kernel.vm.provider "virtualbox" do |vb|
            vb.name = "ubuntu-14.04-click-kernel"
            vb.memory = RAM
            vb.cpus = CPUS
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
            vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
            vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
        end
    end

    # --- VM for Traffic Generator ---
    config.vm.define "trafficgen" do |trafficgen|
        trafficgen.vm.box = BOX
        trafficgen.vm.hostname = "trafficgen"

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
