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

# Box for using libvirt as the provider, bento boxes do not support libvirt.
BOX_LIBVIRT = "generic/ubuntu1804"
BOX_LIBVIRT_VER = "2.0.6"

#######################
#  Provision Scripts  #
#######################

# Common bootstrap
$bootstrap= <<-SCRIPT
# Install dependencies
DEBIAN_FRONTEND=noninteractive apt-get update
DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
DEBIAN_FRONTEND=noninteractive apt-get install -y git pkg-config gdb bash-completion htop dfc iperf iperf3 tcpdump

# Add termite terminal infos
wget https://raw.githubusercontent.com/thestinger/termite/master/termite.terminfo -O /home/vagrant/termite.terminfo
tic -x /home/vagrant/termite.terminfo
SCRIPT

$setup_dev_net= <<-SCRIPT
# Enable IP forwarding
sysctl -w net.ipv4.ip_forward=1
SCRIPT

$setup_x11_server_apt= <<-SCRIPT
DEBIAN_FRONTEND=noninteractive apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y xorg openbox xterm
sed -i 's/#X11UseLocalhost yes/X11UseLocalhost no/g' /etc/ssh/sshd_config
systemctl restart sshd.service
SCRIPT

####################
#  Vagrant Config  #
####################

require 'optparse'

# Parse the provider argument
def get_provider
  ret = nil
  opt_parser = OptionParser.new do |opts|
    opts.on("--provider provider") do |provider|
      ret = provider
    end
  end
  opt_parser.parse!(ARGV)
  ret
end
provider = get_provider || "virtualbox"

Vagrant.configure("2") do |config|

  # --- VM for Network Function development/test/benchmarking ---
  config.vm.define "vnf" do |vnf|

    vnf.vm.provider "virtualbox" do |vb|
      vb.name = "vnf"
      vb.memory = RAM
      vb.cpus = CPUS
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.1", "1"]
      vb.customize ["setextradata", :id, "VBoxInternal/CPUM/SSE4.2", "1"]
    end

    vnf.vm.provider "libvirt" do |libvirt|
      libvirt.driver = "kvm"
      libvirt.cpus = CPUS
      libvirt.memory = RAM
    end

    if provider == "virtualbox"
      vnf.vm.box = BOX
      vnf.vm.box_version = BOX_VER
    elsif provider == "libvirt"
      vnf.vm.box = BOX_LIBVIRT
      vnf.vm.box_version = BOX_LIBVIRT_VER
    end

    vnf.vm.hostname="vnf"
    vnf.vm.box_check_update= true
    vnf.vm.synced_folder ".", "/vagrant"

    # vnf.vm.network "private_network", ip: "192.168.10.13", nic_type: "82540EM"
    # vnf.vm.network "private_network", ip: "192.168.10.14", nic_type: "82540EM"

    # Web access to the controller
    vnf.vm.network :forwarded_port, guest: 8181, host: 18181
    vnf.vm.provision :shell, inline: $bootstrap, privileged: true
    vnf.vm.provision :shell, inline: $setup_x11_server_apt, privileged: true
    # Command line tool to install Linux kernel
    vnf.vm.provision :shell, path: "./script/install_ukuu.sh", privileged: true

    # Always run this when use `vagrant up`
    # - Drop the system caches to allocate hugepages
    vnf.vm.provision :shell, privileged: false, run: "always", inline: <<-SHELL
        echo 3 | sudo tee /proc/sys/vm/drop_caches
        cd /vagrant/ffpp/util || exit
        sudo bash ./setup_hugepage.sh
    SHELL

    vnf.ssh.forward_agent = true
    vnf.ssh.forward_x11 = true
  end

end
