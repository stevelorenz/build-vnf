# -*- mode: ruby -*-
# vi: set ft=ruby :
# About: Vagrant file for the development and emulation environment
#
#        MARK: All evaluation measurements SHOULD be performed on our practical
#        OpenStack cloud testbed.

###############
#  Variables  #
###############

CPUS = 2
RAM = 2048

# Use Ubuntu by default
UBUNTU_LTS = "bento/ubuntu-16.04"
# MARK: Some tools like containernet can not be installed on the latest verision of Ubuntu with built in installation script
UBUNTU_LTS_LATEST = "bento/ubuntu-18.04"
UBUNTU_1404 = "bento/ubuntu-14.04"

ARCH_LATEST = "archlinux/archlinux"

#######################
#  Provision Scripts  #
#######################

# Common bootstrap
$bootstrap_apt= <<-SCRIPT
# Install dependencies
sudo apt update
sudo apt install -y git pkg-config gdb
sudo apt install -y bash-completion htop dfc
sudo apt install -y iperf iperf3 tcpdump

# Add termite infos
wget https://raw.githubusercontent.com/thestinger/termite/master/termite.terminfo -O ~/termite.terminfo
tic -x ~/termite.terminfo

# Get zuo's dotfiles
git clone https://github.com/stevelorenz/dotfiles.git ~/dotfiles
cp ~/dotfiles/tmux/tmux.conf ~/.tmux.conf
cp -r ~/dotfiles/vim ~/.vim
cp ~/dotfiles/vim/vimrc_tiny.vim ~/.vimrc
SCRIPT

$setup_devstack= <<-SCRIPT
# Add a stack user
sudo useradd -s /bin/bash -d /opt/stack -m stack
echo "stack ALL=(ALL) NOPASSWD: ALL" | sudo tee /etc/sudoers.d/stack
# Switch to stack user
sudo -i -u stack \
  bash -c ' git clone https://git.openstack.org/openstack-dev/devstack /opt/stack/devstack &&
            cd /opt/stack/devstack &&
            git checkout -b stable/pike&&
            cp /vagrant/openstack/dev_all_in_one.conf /opt/stack/devstack/local.conf &&
            git clone https://github.com/stevelorenz/nova.git /opt/stack/nova &&
            git checkout -b pike-dev-zuo origin/pike-dev-zuo
          '
SCRIPT

$setup_dev_kernel_apt= <<-SCRIPT
# Get source code
git clone https://github.com/stevelorenz/linux.git $HOME/linux
cd $HOME/linux || exit
# Mark:
#     - Support for AF_XDP since 4.18
LINUX_KERNEL_VERSION=v4.18-rc4
git fetch origin LINUX_KERNEL_VERSION
git checkout -b LINUX_KERNEL_VERSION LINUX_KERNEL_VERSION

## Build and install the kernel from source code
## An alternative is to use compiled binaries from z.B. Ubuntu kernel mainline.
# Install dependencies
sudo apt install -y libncurses-dev bison build-essential flex
# Configure the kernel
#     1. Copy the configure file from current kernel
#     zcat /proc/config.gz ./.config
#     2. Use menuconfig
#     make menuconfig
# Compile and install the kernel
# sudo make -j 2
# sudo make modules_install
# sudo make install
SCRIPT

$setup_dev_net= <<-SCRIPT
# Enable IP forwarding
sysctl -w net.ipv4.ip_forward=1
SCRIPT

$setup_x11_server_apt= <<-SCRIPT
sudo apt update
sudo apt install -y xorg
sudo apt install -y openbox
SCRIPT

# Setup a minimal emulation environment for network softwarization
$setup_netsoft_apt= <<-SCRIPT
echo "Install Mininet..."
git clone git://github.com/mininet/mininet ~/mininet
cd ~/mininet/
git checkout -b 2.2.2 2.2.2
bash ./util/install.sh -nfv

echo "Install Ryu SDN controller..."
sudo apt install -y gcc python-dev libffi-dev libssl-dev libxml2-dev libxslt1-dev zlib1g-dev python-pip
git clone git://github.com/osrg/ryu.git ~/ryu
cd ~/ryu
git checkout -b v4.29 v4.29
pip install .
SCRIPT


####################
#  Vagrant Config  #
####################

Vagrant.configure("2") do |config|

  # --- VM for Traffic Generator ---
  # MARK: To be tested: MoonGen, Trex
  config.vm.define "trafficgen" do |trafficgen|
    trafficgen.vm.box = UBUNTU_LTS
    trafficgen.vm.hostname = "trafficgen"

    trafficgen.vm.network "private_network", ip: "10.0.0.13", mac: "0800271e2db3"
    trafficgen.vm.network "private_network", ip: "10.0.0.14", mac: "080027e1f17d"
    trafficgen.vm.provision :shell, inline: $bootstrap_apt

    trafficgen.vm.provider "virtualbox" do |vb|
      # Set easy to remember VM name
      vb.name = "ubuntu-16.04-trafficgen"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--nictype2", "virtio"]
      vb.customize ["modifyvm", :id, "--nictype3", "virtio"]
    end
  end

  # --- VM for DPDK development ---
  config.vm.define "dpdk" do |dpdk|
    dpdk.vm.box = UBUNTU_LTS
    dpdk.vm.hostname = "dpdk"
    # Create private networks, which allows host-only access to the machine using a specific IP.
    # This option is needed otherwise the Intel DPDK takes over the entire adapter
    # 10.0.0.11 is always used for receiving packets ---> Use fix MAC address
    dpdk.vm.network "private_network", ip: "10.0.0.11", mac: "080027baf4c6"
    dpdk.vm.network "private_network", ip: "10.0.0.12", mac: "0800273c9768"
    dpdk.vm.network "private_network", ip: "10.0.0.33", mac: "0800273c99c4"
    dpdk.vm.provision :shell, inline: $bootstrap_apt
    dpdk.vm.provision :shell, inline: $setup_dev_net

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
    bcc.vm.box = UBUNTU_LTS
    bcc.vm.hostname = "bcc"

    # MARK: Virtio supports XDP with Kernel version after 4.10
    bcc.vm.network "private_network", ip: "10.0.0.15", mac: "080027d49335",
      nic_type: "82540EM"
    bcc.vm.network "private_network", ip: "10.0.0.16", mac: "080027d66961",
      nic_type: "82540EM"
    bcc.vm.provision :shell, inline: $bootstrap_apt

    bcc.vm.provider "virtualbox" do |vb|
      vb.name = "ubuntu-16.04-bcc"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

  # --- VM for Linux Kernel development ---
  config.vm.define "kernel_latest" do |kernel_latest|
    kernel_latest.vm.box = UBUNTU_LTS
    kernel_latest.vm.hostname = "kernellatest"

    kernel_latest.vm.network "private_network", ip: "10.0.0.19",
      nic_type: "82540EM"
    kernel_latest.vm.network "private_network", ip: "10.0.0.20",
      nic_type: "82540EM"
    kernel_latest.vm.provision :shell, inline: $bootstrap_apt
    # kernel_latest.vm.provision :shell, inline: $setup_dev_kernel_apt

    kernel_latest.vm.provider "virtualbox" do |vb|
      vb.name = "ubuntu-16.04-kernellatest"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

  # --- VM for Network Softwarization (NetSoft) Emulator ---
  # MARK: This VM is used for development, with Mininet and Ryu SDN controller installed
  config.vm.define "netsoftemu" do |netsoftemu|
    netsoftemu.vm.box = UBUNTU_LTS
    netsoftemu.vm.hostname = "netsoftemu"

    netsoftemu.vm.network "private_network", ip: "10.0.0.23",
      nic_type: "82540EM"
    netsoftemu.vm.network "private_network", ip: "10.0.0.24",
      nic_type: "82540EM"
    # Web access to the controller
    netsoftemu.vm.network :forwarded_port, guest: 8181, host: 18181
    netsoftemu.vm.provision :shell, inline: $bootstrap_apt
    netsoftemu.vm.provision :shell, inline: $setup_dev_net
    netsoftemu.vm.provision :shell, inline: $setup_netsoft_apt
    # Enable X11 forwarding
    netsoftemu.vm.provision :shell, inline: $setup_x11_server_apt
    netsoftemu.ssh.forward_agent = true
    netsoftemu.ssh.forward_x11 = true

    netsoftemu.vm.provider "virtualbox" do |vb|
      vb.name = "ubuntu-16.04-netsoftemu"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

  # --- VM for Container-based NetSoft Emulator ---
  config.vm.define "netsoftemu4container" do |netsoftemu4container|
    netsoftemu4container.vm.box = UBUNTU_LTS
    netsoftemu4container.vm.hostname = "netsoftemu4container"

    netsoftemu4container.vm.network "private_network", ip: "10.0.0.17", mac:"0800278794f9",
      nic_type: "82540EM"
    netsoftemu4container.vm.network "private_network", ip: "10.0.0.18", mac:"080027bae814",
      nic_type: "82540EM"
    netsoftemu4container.vm.network :forwarded_port, guest: 8888, host: 8888
    netsoftemu4container.vm.provision :shell, inline: $bootstrap_apt
    # Enable X11 forwarding
    netsoftemu4container.vm.provision :shell, inline: $setup_x11_server_apt
    netsoftemu4container.ssh.forward_agent = true
    netsoftemu4container.ssh.forward_x11 = true

    netsoftemu4container.vm.provider "virtualbox" do |vb|
      vb.name = "ubuntu-16.04-netsoftemu4container"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

  # --- VM for trying or testing tools with the latest version ---
  # For example, the latest version of OVS has support for NSH.
  config.vm.define "trylatest" do |trylatest|
    trylatest.vm.box = UBUNTU_LTS
    trylatest.vm.hostname = "trylatest"

    trylatest.vm.network "private_network", ip: "10.0.0.31", nic_type: "82540EM"
    trylatest.vm.provision :shell, inline: $bootstrap_apt
    trylatest.vm.provision :shell, inline: $setup_x11_server_apt
    # Enable X11 forwarding
    trylatest.ssh.forward_agent = true
    trylatest.ssh.forward_x11 = true

    trylatest.vm.provider "virtualbox" do |vb|
      vb.name = "ubuntu-16.04-trylatest"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

end
