# -*- mode: ruby -*-
# vi: set ft=ruby :

###############
#  Variables  #
###############

CPUS = 2
RAM = 2048

# Use Ubuntu LTS for development
BOX = "bento/ubuntu-18.04"
BOX_VER = "201906.18.0"

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
DEBIAN_FRONTEND=noninteractive sudo apt-get update
DEBIAN_FRONTEND=noninteractive sudo apt-get upgrade -y
DEBIAN_FRONTEND=noninteractive sudo apt-get install -y git pkg-config gdb bash-completion htop dfc iperf iperf3 tcpdump

# Add termite terminal infos
wget https://raw.githubusercontent.com/thestinger/termite/master/termite.terminfo -O /home/vagrant/termite.terminfo
tic -x /home/vagrant/termite.terminfo

# Use zuo's tmux config
git clone https://github.com/stevelorenz/dotfiles.git /home/vagrant/dotfiles
cp /home/vagrant/dotfiles/tmux/tmux.conf /home/vagrant/.tmux.conf
SCRIPT

$setup_dev_net= <<-SCRIPT
# Enable IP forwarding
sudo sysctl -w net.ipv4.ip_forward=1
SCRIPT

$setup_x11_server_apt= <<-SCRIPT
DEBIAN_FRONTEND=noninteractive sudo apt-get update
DEBIAN_FRONTEND=noninteractive sudo apt-get install -y xorg openbox xterm
SCRIPT

####################
#  Vagrant Config  #
####################

Vagrant.configure("2") do |config|

  # --- VM to test Software Traffic Generators ---
  # MARK: To be tested: MoonGen, Trex, wrap17
  config.vm.define "trafficgen" do |trafficgen|
    trafficgen.vm.box = BOX
    trafficgen.vm.box_version = BOX_VER
    trafficgen.vm.hostname = "trafficgen"

    trafficgen.vm.network "private_network", ip: "192.168.10.11", mac:"080027601711",
      nic_type: "82540EM"
    trafficgen.vm.network "private_network", ip: "192.168.10.12", mac:"080027601712",
      nic_type: "82540EM"
    trafficgen.vm.provision :shell, inline: $bootstrap_apt, privileged: false

    trafficgen.vm.provider "virtualbox" do |vb|
      vb.name = "trafficgen"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--nicpromisc2", "allow-all"]
      vb.customize ["modifyvm", :id, "--nicpromisc3", "allow-all"]
    end
  end

  # --- VM for VNF development ---
  config.vm.define "vnf" do |vnf|
    vnf.vm.box = BOX
    vnf.vm.box_version = BOX_VER
    vnf.vm.hostname = "vnf"
    vnf.vm.box_check_update= false

    vnf.vm.network "private_network", ip: "192.168.10.13", nic_type: "82540EM"
    vnf.vm.network "private_network", ip: "192.168.10.14", nic_type: "82540EM"
    # Web access to the controller
    vnf.vm.network :forwarded_port, guest: 8181, host: 18181
    vnf.vm.provision :shell, inline: $bootstrap_apt, privileged: false
    vnf.vm.provision :shell, inline: $setup_x11_server_apt, privileged: false
    vnf.vm.provision :shell, privileged: false, inline: <<-SHELL
        cd /vagrant/script || exit
        bash ./install_docker.sh
        cd /vagrant/ffpp/util || exit
        sudo python3 ./run_dev_container.py build_image
    SHELL
    # Always run this when use `vagrant up`
    # - Drop the system caches to allocate hugepages
    vnf.vm.provision :shell, privileged: false, run: "always", inline: <<-SHELL
        echo 3 | sudo tee /proc/sys/vm/drop_caches
        cd /vagrant/ffpp/util || exit
        sudo bash ./setup_hugepage.sh
    SHELL

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
    vnfnext.vm.box = BOX
    vnfnext.vm.box_version = BOX_VER
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

  # --- VM for OpenNetVM development: A high performance container-based NFV platform from GW and UCR. ---
  # WARN: This is experimental
  config.vm.define "opennetvm" do |opennetvm|
    opennetvm.vm.box = BOX
    opennetvm.vm.box_version = BOX_VER
    opennetvm.vm.hostname = "opennetvm"

    opennetvm.vm.network "private_network", ip: "192.168.10.17", mac:"080027601717",
      nic_type: "82540EM"
    opennetvm.vm.network "private_network", ip: "192.168.10.18", mac:"080027601718",
      nic_type: "82540EM"
    opennetvm.vm.provision :shell, inline: $bootstrap_apt, privileged: false
    opennetvm.vm.provision :shell, path: "./script/install_opennetvm.sh", privileged: false
    opennetvm.vm.provision :shell, path: "./script/install_docker.sh", privileged: false

    # Always run this when use `vagrant up`
    # Setup opennetvm environment: hugepages, bind interface to DPDK etc.
    # - The PCI address of the interface can be checked with `lspci`, here the eth1 of the Vagrant VM will be bind to DPDK
    opennetvm.vm.provision :shell, privileged: false, run: "always", inline: <<-SHELL
        echo 3 | sudo tee /proc/sys/vm/drop_caches
        export ONVM_HOME=$HOME/openNetVM
        export RTE_SDK=$ONVM_HOME/dpdk
        export RTE_TARGET=x86_64-native-linuxapp-gcc
        sudo ip link set eth1 down
        export ONVM_NIC_PCI=" 00:08.0 "
        cd $HOME/openNetVM/scripts || exit
        bash ./setup_environment.sh
    SHELL

    opennetvm.vm.provider "virtualbox" do |vb|
      vb.name = "opennetvm"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
      vb.customize ["modifyvm", :id, "--nicpromisc2", "allow-all"]
      vb.customize ["modifyvm", :id, "--nicpromisc3", "allow-all"]
      # By default, OpenNetVM's main components run in busy-polling mode.
      # no matter how much CPU is used in the VM, no more than 50% would be used on the host machine.
      vb.customize ["modifyvm", :id, "--cpuexecutioncap", "50"]
    end
  end

end
