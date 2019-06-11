# -*- mode: ruby -*-
# vi: set ft=ruby :
# About: Vagrant file for the development and emulation VM environment
#

###############
#  Variables  #
###############

CPUS = 2
RAM = 2048

UBUNTU_LTS = "bento/ubuntu-18.04"
UBUNTU_LTS_LATEST = "bento/ubuntu-18.04"
UBUNTU_1404 = "bento/ubuntu-14.04"

#######################
#  Provision Scripts  #
#######################

# MARK: By default, the provision script runs with privileged user (use sudo
# for all commands) If the provision script requires to switch back to vagrant
# user, the practical solution is to set privileged to false and use sudo for
# root privileges.

# Common bootstrap
$bootstrap_apt= <<-SCRIPT
# Install dependencies
sudo apt-get update
sudo apt-get install -y git pkg-config gdb
sudo apt-get install -y bash-completion htop dfc
sudo apt-get install -y iperf iperf3 tcpdump

# Add termite infos
wget https://raw.githubusercontent.com/thestinger/termite/master/termite.terminfo -O /home/vagrant/termite.terminfo
tic -x /home/vagrant/termite.terminfo

# Get zuo's dotfiles
git clone https://github.com/stevelorenz/dotfiles.git /home/vagrant/dotfiles
cp /home/vagrant/dotfiles/tmux/tmux.conf /home/vagrant/.tmux.conf
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
sudo apt-get install -y libncurses-dev bison build-essential flex
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
sudo sysctl -w net.ipv4.ip_forward=1
SCRIPT

$setup_x11_server_apt= <<-SCRIPT
sudo apt-get update
sudo apt-get install -y xorg
sudo apt-get install -y openbox xterm
SCRIPT

####################
#  Vagrant Config  #
####################

Vagrant.configure("2") do |config|

  # --- VM to test Software Traffic Generators ---
  # MARK: To be tested: MoonGen, Trex, wrap17
  config.vm.define "trafficgen" do |trafficgen|
    trafficgen.vm.box = UBUNTU_LTS
    trafficgen.vm.hostname = "trafficgen"

    trafficgen.vm.network "private_network", ip: "192.168.10.11", mac: "0800271e2d13",
      nic_type: "82540EM"
    trafficgen.vm.network "private_network", ip: "192.168.10.12", mac: "0800271e2d14",
      nic_type: "82540EM"
    trafficgen.vm.provision :shell, inline: $bootstrap_apt, privileged: false

    trafficgen.vm.provider "virtualbox" do |vb|
      # Set easy to remember VM name
      vb.name = "build-vnf-trafficgen"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      # vb.customize ["modifyvm", :id, "--nictype2", "virtio"]
      # vb.customize ["modifyvm", :id, "--nictype3", "virtio"]
    end
  end

  # --- VM for VNF development ---
  config.vm.define "vnf" do |vnf|
    vnf.vm.box = UBUNTU_LTS
    vnf.vm.hostname = "vnf"
    vnf.vm.box_check_update= false

    vnf.vm.network "private_network", ip: "192.168.10.13", nic_type: "82540EM"
    vnf.vm.network "private_network", ip: "192.168.10.14", nic_type: "82540EM"
    # Web access to the controller
    vnf.vm.network :forwarded_port, guest: 8181, host: 18181
    vnf.vm.provision :shell, inline: $bootstrap_apt, privileged: false
    vnf.vm.provision :shell, inline: $setup_x11_server_apt, privileged: false
    vnf.ssh.forward_agent = true
    vnf.ssh.forward_x11 = true

    vnf.vm.provider "virtualbox" do |vb|
      vb.name = "vnf"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      # vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

  # --- VM for VNF-Next development. Try latest kernels/framworks/technologies ---
  config.vm.define "vnfnext" do |vnfnext|
    vnfnext.vm.box = UBUNTU_LTS
    vnfnext.vm.hostname = "vnfnext"

    vnfnext.vm.network "private_network", ip: "192.168.10.15", nic_type: "82540EM"
    vnfnext.vm.network "private_network", ip: "192.168.10.16", nic_type: "82540EM"
    vnfnext.vm.provision :shell, inline: $bootstrap_apt, privileged: false
    vnfnext.vm.provision :shell, inline: $setup_x11_server_apt, privileged: false
    # Enable X11 forwarding
    vnfnext.ssh.forward_agent = true
    vnfnext.ssh.forward_x11 = true

    vnfnext.vm.provider "virtualbox" do |vb|
      vb.name = "vnfnext"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

end
