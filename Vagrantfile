# -*- mode: ruby -*-
# vi: set ft=ruby :
# About: Vagrant file for the development environment
#
#        MARK: All evaluation measurements SHOULD be performed on our practical
#        OpenStack cloud testbed.

###############
#  Variables  #
###############

CPUS = 2
RAM = 2048

BOX = "bento/ubuntu-16.04"
BOX_1404 = "bento/ubuntu-14.04"
BOX_1404_316 = "novael_de/ubuntu-trusty64"

#######################
#  Provision Scripts  #
#######################

# Common bootstrap
$bootstrap= <<-SCRIPT
# Install dependencies
sudo apt update
sudo apt install -y git pkg-config gdb
sudo apt install -y bash-completion htop dfc
sudo apt install -y iperf iperf3

# Add termite infos
wget https://raw.githubusercontent.com/thestinger/termite/master/termite.terminfo -O /home/vagrant/termite.terminfo
tic -x /home/vagrant/termite.terminfo

# Get zuo's dotfiles
git clone https://github.com/stevelorenz/dotfiles.git /home/vagrant/dotfiles
cp /home/vagrant/dotfiles/tmux/tmux.conf /home/vagrant/.tmux.conf
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

$setup_dev_kernel= <<-SCRIPT
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

$setup_x11_server= <<-SCRIPT
sudo apt update
sudo apt install -y xorg
sudo apt install -y openbox
SCRIPT

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
    dpdk.vm.network "private_network", ip: "10.0.0.33", mac: "0800273c99c4"
    dpdk.vm.provision :shell, inline: $bootstrap
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
    bcc.vm.box = BOX
    bcc.vm.hostname = "bcc"

    # MARK: Virtio supports XDP with Kernel version after 4.10
    bcc.vm.network "private_network", ip: "10.0.0.15", mac: "080027d49335",
      nic_type: "82540EM"
    bcc.vm.network "private_network", ip: "10.0.0.16", mac: "080027d66961",
      nic_type: "82540EM"
    bcc.vm.provision :shell, inline: $bootstrap

    bcc.vm.provider "virtualbox" do |vb|
      vb.name = "ubuntu-16.04-bcc"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

  # --- VM for Click modular router development ---
  config.vm.define "click" do |click|
    click.vm.box = BOX
    click.vm.hostname = "click"

    click.vm.network "private_network", ip: "10.0.0.17", mac:"0800278794f9",
      nic_type: "82540EM"
    click.vm.network "private_network", ip: "10.0.0.18", mac:"080027bae814",
      nic_type: "82540EM"
    click.vm.provision :shell, inline: $bootstrap

    click.vm.provider "virtualbox" do |vb|
      vb.name = "ubuntu-16.04-click"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

  # --- VM for Linux Kernel development ---
  config.vm.define "kernel_latest" do |kernel_latest|
    kernel_latest.vm.box = BOX
    kernel_latest.vm.hostname = "kernellatest"

    kernel_latest.vm.network "private_network", ip: "10.0.0.19",
      nic_type: "82540EM"
    kernel_latest.vm.network "private_network", ip: "10.0.0.20",
      nic_type: "82540EM"
    kernel_latest.vm.provision :shell, inline: $bootstrap
    # kernel_latest.vm.provision :shell, inline: $setup_dev_kernel

    kernel_latest.vm.provider "virtualbox" do |vb|
      vb.name = "ubuntu-16.04-kernellatest"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

  # --- VM for Mininet Testbed ---
  config.vm.define "mininettb" do |mininettb|
    mininettb.vm.box = BOX
    mininettb.vm.hostname = "mininettb"

    mininettb.vm.network "private_network", ip: "10.0.0.23",
      nic_type: "82540EM"
    mininettb.vm.network "private_network", ip: "10.0.0.24",
      nic_type: "82540EM"
    # Web access to the controller
    mininettb.vm.network :forwarded_port, guest: 8181, host: 18181
    mininettb.vm.provision :shell, inline: $bootstrap
    mininettb.vm.provision :shell, inline: $setup_dev_net
    mininettb.vm.provision :shell, inline: $setup_x11_server
    # Enable X11 forwarding
    mininettb.ssh.forward_agent = true
    mininettb.ssh.forward_x11 = true

    mininettb.vm.provider "virtualbox" do |vb|
      vb.name = "ubuntu-16.04-mininettb"
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

    trafficgen.vm.network "private_network", ip: "10.0.0.13", mac: "0800271e2db3"
    trafficgen.vm.network "private_network", ip: "10.0.0.14", mac: "080027e1f17d"
    trafficgen.vm.provision :shell, inline: $bootstrap

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

  # --- VM for simulation tasks ---
  config.vm.define "sim" do |sim|
    sim.vm.box = BOX
    sim.vm.hostname = "sim"

    sim.vm.network "private_network", ip: "10.0.0.21",
      nic_type: "82540EM"
    sim.vm.network "private_network", ip: "10.0.0.22",
      nic_type: "82540EM"
    sim.vm.provision :shell, inline: $bootstrap
    sim.vm.provision :shell, inline: $setup_x11_server
    # Enable X11 forwarding
    sim.ssh.forward_agent = true
    sim.ssh.forward_x11 = true

    sim.vm.provider "virtualbox" do |vb|
      vb.name = "ubuntu-16.04-sim"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--cableconnected1", "on"]
    end
  end

end
